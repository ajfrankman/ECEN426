import base64

from cryptography.fernet import Fernet
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import dh
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
import cryptography.hazmat.primitives.serialization as serialization


def generate_keyset(param_file="dhparam4096.pem"):
    """
    Generates a private and public keyset based on predefined Diffie-Hellman
    parameters.

    Return: A tuple containing the generated private key (first) and public
            key (second). The keys are returned as bytes in PEM format.
    """
    with open(param_file, "rb") as f:
        parameters = serialization.load_pem_parameters(f.read())

    private_key = parameters.generate_private_key()

    private_key_str = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption(),
    )

    public_key_str = private_key.public_key().public_bytes(
        serialization.Encoding.PEM, serialization.PublicFormat.SubjectPublicKeyInfo
    )

    return private_key_str, public_key_str


def create_shared_key(private_key, peer_public_key):
    """
    Based on a private key and a peer's public key, create a shared key using
    Diffie-Hellman. The shared key is ran through a key derivation function
    (HKDF), with a length of 32 and SHA256.

    Return: The shared key as bytes.
    """

    if not isinstance(private_key, bytes):
        private_key = private_key.encode()

    if not isinstance(peer_public_key, bytes):
        peer_public_key = peer_public_key.encode()

    private_key = serialization.load_pem_private_key(private_key, None)
    peer_public_key = serialization.load_pem_public_key(peer_public_key)

    # Create shared key
    shared_key = private_key.exchange(peer_public_key)

    # Perform key derivation
    derived_key = HKDF(
        algorithm=hashes.SHA256(),
        length=32,
        salt=b"\x00",
        info=b"\x00",
    ).derive(shared_key)

    return derived_key


def encrypt(plaintext, key):
    """
    Encrypt plaintext data using the given key. The Fernet algorithm is used
    for encrypting the data.

    Return: Ciphertext as bytes.
    """
    # Convert to bytes
    if not isinstance(plaintext, bytes):
        plaintext = plaintext.encode()

    # Convert key
    key = base64.b64encode(key)

    return Fernet(key).encrypt(plaintext)


def decrypt(ciphertext, key):
    """
    Decrypt ciphertext data using the given key. The Fernet algorithm is used
    for decrypting the data.

    Return: Plaintext as bytes.
    """

    # Convert to bytes
    if not isinstance(ciphertext, bytes):
        ciphertext = ciphertext.encode()

    # Convert key
    key = base64.b64encode(key)

    return Fernet(key).decrypt(ciphertext)
