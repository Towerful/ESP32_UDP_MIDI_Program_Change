#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <MIDI.h>

static void NetworkTask(void *param);
static void DHCPTask(void *param);

#define ETH_SCK GPIO_NUM_11
#define ETH_MISO GPIO_NUM_4
#define ETH_MOSI GPIO_NUM_12
#define ETH_SS GPIO_NUM_10
#define ETH_RST GPIO_NUM_3
#define ETH_VCC GPIO_NUM_1
#define ETH_GND GPIO_NUM_13



#define NETWORK_BROADCAST_IP {239, 128, 128, 69}
#define NETWORK_PORT 7000
#define HOST_NAME "XPatchMidi"


#define MIDI_IN_PIN GPIO_NUM_6
#define MIDI_OUT_PIN GPIO_NUM_7

#define LOOP_DELAY_MS 20

static EthernetUDP UDP;

struct MySettings : public midi::DefaultSettings
{
  static const long BaudRate = 31250;
};
MIDI_CREATE_CUSTOM_INSTANCE(HardwareSerial, Serial2, MIDI, MySettings);

void setup()
{
  // this is a hack cause of pin alignments
  digitalWrite(ETH_VCC, HIGH);
  digitalWrite(ETH_GND, LOW);

  SPI.begin(ETH_SCK, ETH_MISO, ETH_MOSI, ETH_SS);
  uint8_t MAC_Address[6];
  esp_read_mac(MAC_Address, esp_mac_type_t::ESP_MAC_ETH);
  Ethernet.init(ETH_SS);
  Ethernet.begin(MAC_Address);
  UDP.beginMulticast(NETWORK_BROADCAST_IP, NETWORK_PORT);
  Serial2.setPins(MIDI_IN_PIN, MIDI_OUT_PIN);
  MIDI.begin(MIDI_CHANNEL_OFF);

  static TaskHandle_t NetworkTaskHandle;
  xTaskCreate(NetworkTask, "Network Task", 4096, (void *)1, 1, &NetworkTaskHandle);

  static TaskHandle_t DHCPTaskHandle;
  xTaskCreate(DHCPTask, "DHCP Task", 4096, (void *)1, 1, &DHCPTaskHandle);
}

void loop()
{
  // put your main code here, to run repeatedly:
}

static void NetworkTask(void *param)
{
  TickType_t i;
  while (1)
  {
    i = xTaskGetTickCount();
    int available = UDP.parsePacket();
    if (available)
    {
      byte value[2];
      UDP.read(value, 2);
      // channel, programValue
      MIDI.sendProgramChange(value[1], value[0]);
    }
    xTaskDelayUntil(&i, pdMS_TO_TICKS(LOOP_DELAY_MS));
  }
}

static void DHCPTask(void *param)
{
  TickType_t i;
  while (1)
  {
    i = xTaskGetTickCount();
    Ethernet.maintain();
    xTaskDelayUntil(&i, pdMS_TO_TICKS(20000));
  }
}