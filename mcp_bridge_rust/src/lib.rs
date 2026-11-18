#![deny(unsafe_op_in_unsafe_fn)]

pub mod auth;
#[cfg(windows)]
pub mod bridge;
#[cfg(windows)]
pub mod native;
pub mod wire;
