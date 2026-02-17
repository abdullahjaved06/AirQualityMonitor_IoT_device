#include "certificates.h"

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_key_mgmt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(CERTS);

static const char device_cert[] =
    "-----BEGIN CERTIFICATE-----\n"
"MIIDWTCCAkGgAwIBAgIUU+YD41SHplNdwbx8ccPZxIhEwIMwDQYJKoZIhvcNAQEL\n"
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n"
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI2MDIxNTE4MzEz\n"
"MVoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n"
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMDaOuXq4ZpW4pS7LV7T\n"
"Xlk7haTcoSZVU4aZxQmp0G+tRCF4uvLQfgzeQEwem4sslnN9w9uh8vc9vnLiCx2E\n"
"tFdP+qxmu7JiIeqeHuwM4dRLQ6AhGI8swlUMF9Dg0qFr+xgD9eY8fOhNoAYtdnbO\n"
"oF3OO1oSdYOrg+tn0VewAmzVT3vrcP3oj0ogOPY2sh5AK7zrqB+Kz7b8oilj2COE\n"
"xN1D4P6w/wJoenCnFbUufXNEaW4+9HaShr35QnJq85DELMUaA4N+0QF41abcJ3+c\n"
"W8EUVvKAHl4ntk6I7FiHgF3Lqr1eKowf2TBdL9qHOcbryri5f6eAIOudHIc4lpzg\n"
"0IECAwEAAaNgMF4wHwYDVR0jBBgwFoAUK9SG3LXxjVOjlxUBZKyPQyp+ucEwHQYD\n"
"VR0OBBYEFDGPUzSQ4MhaBYIGpwrXgH2+KoVgMAwGA1UdEwEB/wQCMAAwDgYDVR0P\n"
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQBO4DV3Rdmx6MphAxBVXAUpGGNW\n"
"ilLVIWQXqFhDogDK/E0wy2zbwFlTabndParLK5HYVHejxTKCDrSH8TTH4q/adAGL\n"
"fYWVlYAlFmnjpjLDYNGYGlcUagd3XwK3uJCl6uxzb6/27gOsiUvnrE0cNAqaLvEx\n"
"H0CPOKOTLhzW6gxHsrJH+emDWnQ0HeCPT73gjWSAxh6pBHFc+bPEFiwRLXM0Pkjm\n"
"eKtVWDJJdyJ6VrFLHlop4lDuwyj8pzXKYxF/uSMDLV2cxow/lCmL04xyx8cMZC/8\n"
"MjICwc3HorVzKxgq2Z9Gq3bs/vI7gHI464Zec8Ni8bNw1OShkbXyBZ3uJr0h\n"
"-----END CERTIFICATE-----\n";




static const char private_key[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpAIBAAKCAQEAwNo65erhmlbilLstXtNeWTuFpNyhJlVThpnFCanQb61EIXi6\n"
"8tB+DN5ATB6biyyWc33D26Hy9z2+cuILHYS0V0/6rGa7smIh6p4e7Azh1EtDoCEY\n"
"jyzCVQwX0ODSoWv7GAP15jx86E2gBi12ds6gXc47WhJ1g6uD62fRV7ACbNVPe+tw\n"
"/eiPSiA49jayHkArvOuoH4rPtvyiKWPYI4TE3UPg/rD/Amh6cKcVtS59c0Rpbj70\n"
"dpKGvflCcmrzkMQsxRoDg37RAXjVptwnf5xbwRRW8oAeXie2TojsWIeAXcuqvV4q\n"
"jB/ZMF0v2oc5xuvKuLl/p4Ag650chziWnODQgQIDAQABAoIBAFM8XXtqmyDAK19C\n"
"QlzRIcGlvRLg3vTqkvhfWfnHv1zUDbncn+O5NQ5cxqeT9lJlVjJWr7gV+AXaMl7n\n"
"TSDaa/PbYzgRxyVA0Z6vzGzZSocUTQXKAw2Ype+LHqRaxM5DCsbcvr82kDq7djv0\n"
"V1MD75dNMbS4jtvbpT5vOp18LC/ISBTJ/voDaORz5k6WM19RqZeEzJ7Kfo1ZpyVM\n"
"h7e2XsfMtnHw0xMj2FpvUUooj3oIsSJAXtTd5VQMDSfNZGRmts58eXbISfxarLT0\n"
"zCwhAxshstDPpta0uUPwsMz85wKXTsm2h1T7YYcVNf9ldqwLa/Sndktxe9+kMpvn\n"
"3iTaz/kCgYEA+4XVzEH9PFmwvrK4K1Ef2efkMw190GuToZuBF4yoH0Q8Fw5z2DDI\n"
"OQz6AtPNMxdEje9BdIDezc0/2hCWA/61QeCR6O2eXOgWpEBcLVqDrLq/2u5wYDe3\n"
"uQz91gd0IPb8+fhjY30KXprk6ZMrMCVDKq0ApBSD8FUJ2SFkrKcoT18CgYEAxEkK\n"
"OVd+CoTfiG6kW5AQEzFNuTSq5Y59bvqQ6MVctKyaHRsXE49VGvoULbXnOMcw5mug\n"
"eEq7r12R9IUkO9lreWhidF9OFVFBPWeShS/kUy4JNB1hXNGaU87koBEgzbHL1hDm\n"
"kU3L/CCWCmVNnFM1d7wMKs+3JC3OmHPepbHxTB8CgYAn2jzV8F6/gTaQAKFNu7zg\n"
"Oe3eaPTWYTUzFgCOSqsYySb8QD36s0AGShgS+pw4zCcAljg4zXKACVVkp9wdJe5y\n"
"UbLMxVmiPuPXgpsYVI8ofy6G8eC8XheuKnDNyMCNuJS8xq54xQcvljtPBxKAvma2\n"
"DFNdrCJ7of29eZ0J/jJU4QKBgQCNf0nLhHLsClVJ+Niq2PN9onExLfJ0gX+S3TZR\n"
"VtU5uaNvj/PWueDUDas2OIdyusVZlgScMuORy5ZH+yLfsiBz6PfwhDO50lWBeoR5\n"
"Vsj13Z3s37EsRD1IZUES0sYfAii7LHvKC5cdLjB7VgPYyXMl2X277vlLL2pbJExA\n"
"jkIwaQKBgQD6zp4T5V1JaC3OQalPjVLt2kU0a/EOusyVHPSD/rHlbi90h59Ae1uR\n"
"prkRYZxyWXsBzz+ZcpbrY8T/PKHSZNc72uyFTKKqiAZKD/Q9grlcRYBXqJqRou1s\n"
"IW/qBGPGscpxqDviGqVsv3xLuukqiUv+AYKFUqW0WmhKvUURTr/HpQ==\n"
"-----END RSA PRIVATE KEY-----\n";



static const char root_ca[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
"rqXRfboQnoZsG4q5WTP468SQvvG5\n"
"-----END CERTIFICATE-----\n";

  
#define MQTT_SEC_TAG 30

const char *modem_key_mgmt_cred_type_str(enum modem_key_mgmt_cred_type key)
{
    switch (key)
    {
    case MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN:
        return "MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN";
    case MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT:
        return "MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT";
    case MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT:
        return "MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT";
    default:
        return "UNKNOWN_CRED_TYPE";
    }
}

static int write_certificates_to_modem(uint32_t TAG, enum modem_key_mgmt_cred_type key, const void *buf, size_t len)
{
    int err;
    bool exists;

    err = modem_key_mgmt_exists(TAG, key, &exists);
    if (err)
    {
        LOG_ERR("Failed to check for certificates [ %s ] err %d\n", modem_key_mgmt_cred_type_str(key), err);
        return err;
    }

    if (exists)
    {
        /* For the sake of simplicity we delete what is provisioned
         * with our security tag and reprovision our certificate.
         */
        err = modem_key_mgmt_delete(TAG, key);
        if (err)
        {
            LOG_ERR("Failed to delete existing certificate [ %s ], err [%d]\n",
                    modem_key_mgmt_cred_type_str(key), err);
        }
    }

    err = modem_key_mgmt_write(TAG, key, buf, len);
    if (err)
    {
        LOG_ERR("Failed to write certificate [ %s ]to modem\n", modem_key_mgmt_cred_type_str(key));
        return err;
    }
    LOG_INF("Updated CERT [%s]\n\r", modem_key_mgmt_cred_type_str(key));

    return err;
}

int write_device_certs_to_modem(void)
{
    int err;
    err = nrf_modem_lib_init();
    if (err)
    {
        // LOG_ERR("Modem info initialization failed, error: %d", err);
        return err;
    }

    write_certificates_to_modem(MQTT_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, (const void *)root_ca, sizeof(root_ca));

    write_certificates_to_modem(MQTT_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT, (const void *)device_cert, sizeof(device_cert));

    write_certificates_to_modem(MQTT_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT, (const void *)private_key, sizeof(private_key));

    return err;
}
