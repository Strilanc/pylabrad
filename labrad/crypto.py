import base64
import hashlib
import logging
import os

try:
    from twisted.internet import ssl
    TLS = True
except ImportError:
    logging.warning('pyOpenSSL not found. Without encryption you will only be '
                    'able to connect to the labrad manager on localhost.')
    TLS = False

certs_path = os.path.expanduser('~/.labrad/client/certs')
# Location where we store trusted self-signed manager certs.
# The manager cert file for a given hostname is named <hostname>.cert
# Note that real certs signed by public CAs are not stored here,
# because we can validate them with the system's trusted certificates.

def tls_options(hostname, cert_string=None):
    """Get certificate options for TLS connections.

    If we are unable to use TLS because pyOpenSSL is not installed, this will
    raise an exception.

    Args:
        hostname (string): The hostname we are trying to connect to with TLS.
        cert_string (string): A certificate in PEM format. If provided, we
            return CertificateOptions that will trust only the given cert
            (assuming its canonical name attribute matches the given hostname).
            If not provided, we will try to load a suitable certificate for
            this hostname from our store of trusted self-signed certificates,
            and otherwise fall back to just using the trust roots configured
            for the OS to validate the server certificate.

    Returns (twisted.internet.ssl.CertificateOptions):
        Certificate suitable for use by TLS clients to connect to the given
    """
    if not TLS:
        raise Exception('Unable to connect to labrad with TLS. Please install pyOpenSSL.')
    if cert_string is None:
        cert_string = load_cert(hostname)
    if cert_string is None:
        logging.info('No certificate found for host "{}" in {}. Will use '
                     'system certs to verify tls'.format(hostname, certs_path))
        return ssl.optionsForClientTLS(hostname.decode('utf-8'))
    else:
        cert = ssl.Certificate.loadPEM(cert_string)
        return ssl.optionsForClientTLS(hostname.decode('utf-8'), cert)


def save_cert(hostname, cert_string):
    """Save the given self-signed certificate as a trusted certificate.

    Args:
        hostname (string): The hostname for this certificate. Must match the
            canonical name attribute (CN) in the certificate itself. We do not
            check this here, but if it is not the case, TLS negotiation using
            this cert will fail.
        cert_string (string): Certificate in PEM format.
    """
    # sanity check that this is a valid PEM string
    cert = ssl.Certificate.loadPEM(cert_string)

    fname = hostname + '.cert'
    full_name = os.path.join(certs_path, fname)
    with open(full_name, 'w') as f:
        f.write(cert_string)


def load_cert(hostname):
    """Load a trusted certificate for the given hostname.

    Args:
        hostname(string): The hostname for which we want a certificate.

    Returns (string | None):
        A certificate for the specified hostname as a PEM-encoded string. If no
        certificate is found for the given hostname, we return None.
    """
    for fname in os.listdir(certs_path):
        full_name = os.path.join(certs_path, fname)
        if not os.path.isfile(full_name):
            continue
        host, ext = os.path.splitext(fname)
        if host == hostname and ext == '.cert':
            with open(full_name) as f:
                return f.read()
    else:
        return None


def fingerprint(cert_string, digest=hashlib.sha1):
    """Get a human-readable digest of the given certificate string.

    Certificate fingerprints are computed from the DER-encoded (binary) form of
    the certificate, while PEM is a text format containing the base64-encoded
    DER bytes. So we strip off boilerplate/whitespace and base64-decode before
    computing the digest.

    Args:
        cert_string (string): PEM-encoded certificate.
        digest: the hashlib algorithm to use; a callable that returns an
            instance of a message digest algorithm.

    Returns (string):
        Human-readable hex version of the certificate digest in the form:
        'AA:BB:CC:...:FF'.
    """
    cert_b64 = (cert_string.replace('-----BEGIN CERTIFICATE-----', '')
                           .replace('-----END CERTIFICATE-----', '')
                           .replace('\n', ''))
    cert_bytes = base64.b64decode(cert_b64)
    m = digest()
    m.update(cert_bytes)
    digest = m.hexdigest()

    def grouped(s, n=2):
        while s:
            yield s[:n]
            s = s[n:]

    return ':'.join(g.upper() for g in grouped(digest, 2))
