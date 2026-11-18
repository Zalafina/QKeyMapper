use std::{future::Future, time::Duration};

use rmcp::{
    RoleServer, ServerHandler, ServiceExt,
    model::{ClientJsonRpcMessage, ClientNotification, ServerJsonRpcMessage},
    service::{QuitReason, ServerInitializeError},
    transport::{Transport, async_rw::AsyncRwTransport, stdio},
};
use tokio::{
    io::{Stdin, Stdout},
    runtime::Builder,
};
use tokio_util::sync::CancellationToken;

type Pipes = AsyncRwTransport<RoleServer, Stdin, Stdout>;

struct StdioTransport {
    pipes: Pipes,
    output_failed: CancellationToken,
}

impl Transport<RoleServer> for StdioTransport {
    type Error = <Pipes as Transport<RoleServer>>::Error;

    fn send(
        &mut self,
        message: ServerJsonRpcMessage,
    ) -> impl Future<Output = Result<(), Self::Error>> + Send + 'static {
        let send = self.pipes.send(message);
        let output_failed = self.output_failed.clone();
        async move {
            let result = send.await;
            if result.is_err() {
                output_failed.cancel();
            }
            result
        }
    }

    async fn receive(&mut self) -> Option<ClientJsonRpcMessage> {
        loop {
            let message = self.pipes.receive().await?;
            // rmcp 3.10 can close during bootstrap if it sees an unknown notification.
            if !matches!(
                &message,
                ClientJsonRpcMessage::Notification(notification)
                    if matches!(notification.notification, ClientNotification::CustomNotification(_))
            ) {
                return Some(message);
            }
        }
    }

    async fn close(&mut self) -> Result<(), Self::Error> {
        self.pipes.close().await
    }
}

pub fn run(handler: impl ServerHandler, max_line_bytes: usize) -> Result<(), &'static str> {
    let runtime = Builder::new_current_thread()
        .enable_time()
        .build()
        .map_err(|_| "Could not start the MCP runtime")?;
    let output_failed = CancellationToken::new();
    let result = runtime.block_on(async {
        let (read, write) = stdio();
        let transport = StdioTransport {
            pipes: Pipes::new(read, write).with_max_line_length(max_line_bytes),
            output_failed: output_failed.clone(),
        };
        match handler
            .serve_with_ct(transport, output_failed.child_token())
            .await
        {
            Ok(service) => match service.waiting().await {
                Ok(QuitReason::Closed) => Ok(()),
                _ => Err("MCP stdio service stopped"),
            },
            Err(ServerInitializeError::ConnectionClosed(_)) => Ok(()),
            Err(_) => Err("MCP stdio startup failed"),
        }
    });
    if output_failed.is_cancelled() {
        // Tokio's blocking stdin read can outlive the service after stdout breaks.
        runtime.shutdown_timeout(Duration::from_millis(200));
        return Err("MCP stdio output failed");
    }
    result
}
