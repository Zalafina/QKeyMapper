use std::num::{NonZeroU32, NonZeroU64};

use hmac::{Hmac, KeyInit, Mac};
use sha2::Sha256;

#[derive(Clone, Copy, PartialEq, Eq)]
pub struct ProcessStamp {
    pub pid: NonZeroU32,
    pub created: NonZeroU64,
}

#[repr(u8)]
#[derive(Clone, Copy)]
pub enum ProofRole {
    Server = 1,
    Client = 2,
    PairingConfirmation = 3,
}

pub struct AuthBinding<'a> {
    pub instance: &'a [u8; 16],
    pub pair_id: &'a [u8; 16],
    pub epoch: NonZeroU64,
    pub client: ProcessStamp,
    pub server: ProcessStamp,
    pub client_nonce: &'a [u8; 32],
    pub server_nonce: &'a [u8; 32],
}

impl AuthBinding<'_> {
    fn mac(&self, role: ProofRole, key: &[u8; 32]) -> Hmac<Sha256> {
        let mut mac =
            Hmac::<Sha256>::new_from_slice(key).expect("HMAC-SHA256 accepts this key size");
        mac.update(b"QKeyMapper-MCP-AUTH\0");
        mac.update(&[role as u8]);
        mac.update(&1u32.to_be_bytes());
        mac.update(self.instance);
        mac.update(self.pair_id);
        mac.update(&self.epoch.get().to_be_bytes());
        for stamp in [self.client, self.server] {
            mac.update(&stamp.pid.get().to_be_bytes());
            mac.update(&stamp.created.get().to_be_bytes());
        }
        mac.update(self.client_nonce);
        mac.update(self.server_nonce);
        mac
    }

    pub fn proof(&self, role: ProofRole, key: &[u8; 32]) -> [u8; 32] {
        self.mac(role, key).finalize().into_bytes().into()
    }

    pub fn verify(&self, role: ProofRole, key: &[u8; 32], actual: &[u8; 32]) -> bool {
        self.mac(role, key).verify_slice(actual).is_ok()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::wire::hex_bytes;

    #[test]
    fn proofs_match_shared_qt_and_python_vectors() {
        let binding = AuthBinding {
            instance: &hex_bytes("00112233445566778899aabbccddeeff").unwrap(),
            pair_id: &hex_bytes("ffeeddccbbaa99887766554433221100").unwrap(),
            epoch: NonZeroU64::MAX,
            client: ProcessStamp {
                pid: NonZeroU32::new(0x12345678).unwrap(),
                created: NonZeroU64::new(0xFEDCBA9876543210).unwrap(),
            },
            server: ProcessStamp {
                pid: NonZeroU32::new(0x87654321).unwrap(),
                created: NonZeroU64::new(0x0123456789ABCDEF).unwrap(),
            },
            client_nonce: &std::array::from_fn(|i| i as u8),
            server_nonce: &std::array::from_fn(|i| i as u8 + 32),
        };
        let key = std::array::from_fn(|i| i as u8 + 1);
        let server =
            hex_bytes("707f9b7b991818ffac75ed543bf43e5bbc13beaa56a821a496207772369ca8b0").unwrap();
        for (role, expected) in [
            (ProofRole::Server, server),
            (
                ProofRole::Client,
                hex_bytes("832a59307600f9ca81e2a58f282f54ecd6f492e8c149662a15371ac05e359f38")
                    .unwrap(),
            ),
            (
                ProofRole::PairingConfirmation,
                hex_bytes("d0846e2424fc8034f6723ac5d5e7bdc951af20a7de76f1f3a42485fa9a3b824b")
                    .unwrap(),
            ),
        ] {
            assert_eq!(binding.proof(role, &key), expected);
            assert!(binding.verify(role, &key, &expected));
        }
        assert!(!binding.verify(ProofRole::Client, &key, &server));
        let stale = AuthBinding {
            epoch: NonZeroU64::MIN,
            ..binding
        };
        assert!(!stale.verify(ProofRole::Server, &key, &server));
    }
}
