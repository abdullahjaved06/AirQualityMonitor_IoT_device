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
"MIIDWTCCAkGgAwIBAgIUf7f4r9VPZaEDy/GbQ1V4ptSEPUEwDQYJKoZIhvcNAQEL\n"
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n"
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI1MTIxOTE5NDUx\n"
"MVoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n"
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAObJLUdqyizoXIJsqbDr\n"
"ylPhlwjawHPwRhP2dpPab8gZbmut7IRsUFAwb7DtKzU7g93vFOGSoBs86TLJZzRg\n"
"3KSiQQfFQWnHWdHXw7jPQnRHYErGtIBed7LdHOWfjFerZcqY45dQOwSKpI8SV0tK\n"
"esXV9bKTM01OQSGgW/zVa1ZF4e1vGUwn61YxK435VLioncCBAD/TC0KDkPZw/ga+\n"
"RNAvfnc//S2I/epochRXH96RUwVH27A+0mkluETpFv+DtVrxHGWEbmnntTfQdF9f\n"
"BpK3/FtSfjXL+hZj5A4cax1k3ycp3Td2aqMxCX2xrfRhmOY8sP2ZV8d1e18NFhd/\n"
"D/ECAwEAAaNgMF4wHwYDVR0jBBgwFoAU4org8N5iiXPEJpeVSByX+R744dMwHQYD\n"
"VR0OBBYEFHDK+1C240y7uis6PTUV5DMp/favMAwGA1UdEwEB/wQCMAAwDgYDVR0P\n"
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQAZphQYkpcbjO+f9U+m+gVbzm7j\n"
"mo5hJSULdpRTlAPJSwFUU9dCGVHs9BXj7TcvDxrSBLaKT7VgEZoFSfYd1SlWjQQQ\n"
"QqViM3BpVuKUoPBgXO6FTdcGx2V6RErN+eMop8VohYkaFhGnv3kR9WCQ5P3x6vAS\n"
"z5zgJ3TmBMFzf519WMiSUcpyJppLS4Ob/GMMyc8U+GgnxnpLogu33obzf34iZVXx\n"
"PD7MFs6/n1tf+yanIc4iNwMAQ7WHPR0ueUBo+6Ko//DgsUNCPirHKV+FuCaHXLNA\n"
"ZmiYm02ijyA+h1aRvS/hmY4C2EdoVr924kYrJvQSBF/AxZlCQ5KC2FBgoeMr\n"
"-----END CERTIFICATE-----\n";




static const char private_key[] =
    "-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpAIBAAKCAQEA5sktR2rKLOhcgmypsOvKU+GXCNrAc/BGE/Z2k9pvyBlua63s\n"
"hGxQUDBvsO0rNTuD3e8U4ZKgGzzpMslnNGDcpKJBB8VBacdZ0dfDuM9CdEdgSsa0\n"
"gF53st0c5Z+MV6tlypjjl1A7BIqkjxJXS0p6xdX1spMzTU5BIaBb/NVrVkXh7W8Z\n"
"TCfrVjErjflUuKidwIEAP9MLQoOQ9nD+Br5E0C9+dz/9LYj96mhyFFcf3pFTBUfb\n"
"sD7SaSW4ROkW/4O1WvEcZYRuaee1N9B0X18Gkrf8W1J+Ncv6FmPkDhxrHWTfJynd\n"
"N3ZqozEJfbGt9GGY5jyw/ZlXx3V7Xw0WF38P8QIDAQABAoIBAQDlfzryoQRJoguU\n"
"eyBH2kJKJbQ+zuHAqTfW1ClYoEi2cGu40qy3hspa47+97isgdX2NbhmSs29ZhrnT\n"
"kip6ELR5VwxaANMqsF3maaytFBXecbgUxAJtAQQBLxZ0VbOG3t+Ll3wDVXpK6t/3\n"
"kmKScY8jOsBogy7p+h6UpNW02Doz/8ijSugrfHTpKES79TxRL5HE0A8MztX0UIMm\n"
"jPx0Nqq6onvWOI1na6WzyJZMXOGWYjiTqIkYiDwGgvkSMZjCP2F6NzxTR/9Npngk\n"
"QJ9SXcgMO/4UIrkPk+7MVihFAHqRCAq/RsUOKZpmJ/GRqZe4kiv6TEyD2TymzF2J\n"
"iqJt6p7RAoGBAPqnJ9BvJsG2npsbKbyU+Vu84Ke1LISd9x+ECrvRHRLg4nuHa5zZ\n"
"m8p6kdU90OVOpKpFWD0NtGUdHcYsztKcDtIVDy5F9DXrbQtui0p3x575nQNNjPXE\n"
"6WeQorjfGoAyQgLBy5e3ediQ65hhMyochmxCtTNiwWwNSSc9BYMKUlEFAoGBAOu1\n"
"hlpSgRCWQxxF8adtDvyMMz4fHVFARgGNzTD2141+iwfkMGPlCgxx0COGnBvOz4M7\n"
"kManiDaVRmCGYtbohHbBkI+ROvZZhjNx0ba5YWo0zu5SV39bokd6fSaZT7xNKQNj\n"
"pUxv8gp/Nmz5/il00hQfGmdB1UJuwspSorRC+Wb9AoGAAWVe7mVAFQIcXgbHs7os\n"
"rVse8TAsIEz07GMC4Ero24sPF9sIOWZo3LbUCxj+CdjnR998/1INOCbyXIExYbDm\n"
"0Dq3y+0t0AMQp2ilM/P406TWQAd1ioTfO+ltTpRHhIFDJtrHdH/cGN2twjqAW3Kv\n"
"OPkiIzgglaZ5StOnLTSogrUCgYEAqTKQ0t1OkF2Mpwr+QLTkgR0S25DyNpTwq8Ti\n"
"ejd227bujiebJNoQsIYyZo4GGWAHTajAcBlqieP1tOxCnwohrC2eW7Bgpt30m5Ek\n"
"fQnth5OIx0KMVHuuraiblc2PkxfATRKGYawqDqbqpqd9brzQ4GjDR0PWle10Hk/b\n"
"Q5Kf/PECgYB0j/lrJeZTmRh2ZzVVaYOvcPn/SDmWQSk4zQP9dzLKTj+LT5jSXZiN\n"
"lQfjkneltkKmOscLYVNauHawUr2cwWaO4D/tuoDtto/tdBpY2FB8XBuM9A6tTw/R\n"
"oh6PS8aqKznsUowVGL6axoONtIxuAOzeYnUFRplIK9O1nmCwrfnKAw==\n"
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
