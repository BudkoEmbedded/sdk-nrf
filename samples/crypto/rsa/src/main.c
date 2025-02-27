/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <logging/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <crypto_common.h>

LOG_MODULE_REGISTER(rsa, LOG_LEVEL_DBG);

/* ====================================================================== */
/*				Global variables/defines for the RSA example			  */

#define NRF_CRYPTO_EXAMPLE_RSA_TEXT_SIZE (100)
#define NRF_CRYPTO_EXAMPLE_RSA_PUBLIC_KEY_SIZE (140)
#define NRF_CRYPTO_EXAMPLE_RSA_SIGNATURE_SIZE (128)

/* Below text is used as plaintext for signing using RSA . */
static char m_plain_text[NRF_CRYPTO_EXAMPLE_RSA_TEXT_SIZE] = {
	"Example string to demonstrate basic usage of RSA."
};

static char m_pub_key[NRF_CRYPTO_EXAMPLE_RSA_PUBLIC_KEY_SIZE];

static char m_signature[NRF_CRYPTO_EXAMPLE_RSA_SIGNATURE_SIZE];
static char m_hash[32];

static psa_key_handle_t keypair_handle;
static psa_key_handle_t pub_key_handle;
/* ====================================================================== */

int crypto_finish(void)
{
	psa_status_t status;

	/* Destroy the key handle */
	status = psa_destroy_key(keypair_handle);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_destroy_key", status);
		return APP_ERROR;
	}

	status = psa_destroy_key(pub_key_handle);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_destroy_key", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int generate_rsa_keypair(void)
{
	psa_status_t status;
	size_t olen;

	PRINT_MESSAGE("Generating random RSA keypair...");

	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	/* Configure the key attributes */
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_SIGN_HASH);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_RSA_KEY_PAIR);
	psa_set_key_bits(&key_attributes, 1024);

	/* Generate a random keypair. The keypair is not exposed to the application,
	 * we can use it to signing/verification the key handle.
	 */
	status = psa_generate_key(&key_attributes, &keypair_handle);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_generate_key", status);
		return APP_ERROR;
	}

	/* Export the public key */
	status = psa_export_public_key(keypair_handle, m_pub_key, sizeof(m_pub_key), &olen);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_export_public_key", status);
		return APP_ERROR;
	}

	/* After the key handle is acquired the attributes are not needed */
	psa_reset_key_attributes(&key_attributes);

	PRINT_MESSAGE("RSA generated successfully!");

	return APP_SUCCESS;
}

int import_rsa_pub_key(void)
{
	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	/* Configure the key attributes */
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_RSA_PUBLIC_KEY);
	psa_set_key_bits(&key_attributes, 1024);

	status = psa_import_key(&key_attributes, m_pub_key, sizeof(m_pub_key), &pub_key_handle);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_import_key", status);
		return APP_ERROR;
	}

	/* After the key handle is acquired the attributes are not needed */
	psa_reset_key_attributes(&key_attributes);

	return APP_SUCCESS;
}

int sign_message_rsa(void)
{
	uint32_t olen;
	psa_status_t status;

	PRINT_MESSAGE("Signing a message using RSA...");

	/* Compute the SHA256 hash */
	status = psa_hash_compute(
		PSA_ALG_SHA_256, m_plain_text, sizeof(m_plain_text), m_hash, sizeof(m_hash), &olen);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_hash_compute", status);
		return APP_ERROR;
	}

	/* Sign the hash using RSA */
	status = psa_sign_hash(keypair_handle,
			       PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256),
			       m_hash,
			       sizeof(m_hash),
			       m_signature,
			       sizeof(m_signature),
			       &olen);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_sign_hash", status);
		return APP_ERROR;
	}

	PRINT_MESSAGE("Singing was successful!");
	PRINT_HEX("Plaintext", m_plain_text, sizeof(m_plain_text));
	PRINT_HEX("SHA256 hash", m_hash, sizeof(m_hash));
	PRINT_HEX("Signature", m_signature, sizeof(m_signature));

	return APP_SUCCESS;
}

int verify_message_rsa(void)
{
	psa_status_t status;

	PRINT_MESSAGE("Verifying RSA signature...");

	/* Verify the hash */
	status = psa_verify_hash(pub_key_handle,
				 PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256),
				 m_hash,
				 sizeof(m_hash),
				 m_signature,
				 sizeof(m_signature));
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_verify_hash", status);
		return APP_ERROR;
	}

	PRINT_MESSAGE("Signature verification was successful!");

	return APP_SUCCESS;
}

int main(void)
{
	int status;

	PRINT_MESSAGE("Starting the RSA example...");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		PRINT_MESSAGE(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = generate_rsa_keypair();
	if (status != APP_SUCCESS) {
		PRINT_MESSAGE(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = import_rsa_pub_key();
	if (status != APP_SUCCESS) {
		PRINT_MESSAGE(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = sign_message_rsa();
	if (status != APP_SUCCESS) {
		PRINT_MESSAGE(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = verify_message_rsa();
	if (status != APP_SUCCESS) {
		PRINT_MESSAGE(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = crypto_finish();
	if (status != APP_SUCCESS) {
		PRINT_MESSAGE(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	PRINT_MESSAGE(APP_SUCCESS_MESSAGE);

	return APP_SUCCESS;
}
