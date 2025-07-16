ESP32 VESDA p2p System / VESDA-p2p
===
## Intro
  Since nowadays VESDA (Very Early Smoke Detection Apparatus) mostly focus on the smoke and particles detection in the room, in the big picture, this could detecting fire before disasters had happen. But if we need precise detection, for example, in a server room or data center and we need to tell which server is on fire not whether the room is on fire, this is were the traditional VESDA cannot do.  
  
So the basic idea of this project is to use ESP32 and it's ESPNOW feature to structure a p2p network, and then use a ESPNOW to WiFi broker to transmit the datas to the server/dashboard etc.  
  
But why ESPNOW but not transmit the data directly to the server? First of all, every AP has a connect/slave limit, also does WiFi connectivity are not that consistent and reliable. ESPNOW got the low energy and latency advantages among normal 2.4gHz WiFi connections. Also, since it's a p2p network, it's more easy to add nodes to the network and reconstruct the network structure.  

## Architecture/Network Structure
### Sensor Node/Slave
  Basically any esp32 that supports espnow could be the node, hook up any sensor you want and modify the code to match your needs. The sensor used in this repo is the Sharp GP2Y1010AU0F optical dust sensor to detect smoke before the fire.  

### ESPnow2WiFi broker/Master
  By defualt espnow uses channel 1 for the protocol, but the AP may use a different one. So in order not to make the structure too complicated, here we combine two esp32 to make the broker, commuicating over UART(could be modify to SPI for further high speed purposes) s.t. we don't need to worry about channel issues.  
  
The espnow end receives the datas from the nodes/slaves, organizes the datas and send it to the WiFi end, then the data being transmitted to the server/dashboard.  
