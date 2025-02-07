# ESP8266_MQTT

```
docker run -d --name emqx -p 1883:1883 -p 8085:8085 -p 8086:8086 -p 8883:8883 -p 18083:18083 emqx/emqx:latest
```

linux mqtt config

```
sudo apt update
sudo apt install mosquitto mosquitto-clients
```

```
sudo nano /etc/mosquitto/mosquitto.conf
```

### Here’s a basic example of what a Mosquitto configuration file might look like:

```
# Mosquitto configuration file

# Listener configuration
listener 1883
protocol mqtt

# Allow anonymous connections
allow_anonymous true

# Persistence configuration
persistence true
persistence_location /var/lib/mosquitto/

# Logging configuration
log_dest file /var/log/mosquitto/mosquitto.log
log_dest stdout
log_type all

# Security
# Uncomment the following lines to enable password authentication
# password_file /etc/mosquitto/passwd
# allow_anonymous false
```


