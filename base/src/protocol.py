import config


def process_and_send(requests_session, data_str, rssi):
    if not requests_session:
        return

    parts = data_str.split(",")
    fields_line_protocol = []
    payload_json = {}

    for part in parts:
        if ":" in part:
            k, v = part.split(":", 1)
            fields_line_protocol.append(f"{k}={v}")
            try:
                payload_json[k] = float(v)
            except ValueError:
                payload_json[k] = v

    if not fields_line_protocol:
        return

    fields_line_protocol.append(f"rssi={rssi}")
    payload_json["rssi"] = rssi

    field_str = ",".join(fields_line_protocol)
    line_protocol = f"{config.INFLUX_MEASUREMENT} {field_str}"

    influx_url = f"{config.INFLUX_URL}/api/v2/write?org={config.INFLUX_ORG}&bucket={config.INFLUX_BUCKET}&precision=s"
    influx_headers = {
        "Authorization": f"Token {config.INFLUX_TOKEN}",
        "Content-Type": "text/plain; charset=utf-8",
    }

    try:
        resp_influx = requests_session.post(
            influx_url, headers=influx_headers, data=line_protocol
        )
        if config.DEBUG:
            print(f"influx: {resp_influx.status_code}")
        resp_influx.close()
    except Exception as e:
        if config.DEBUG:
            print(f"influx: {e}")

    try:
        resp_gw = requests_session.post(config.GATEWAY_URL, json=payload_json)
        if config.DEBUG:
            print(f"gateway: {resp_gw.status_code}")
        resp_gw.close()
    except Exception as e:
        if config.DEBUG:
            print(f"gateway: {e}")
