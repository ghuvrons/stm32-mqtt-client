/*
 * mqtt.c
 *
 *  Created on: Sep 29, 2021
 *      Author: janoko
 */

#include "mqtt_packet.h"
#include "mqtt.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "debugger.h"


static void processPacket(MQTT_Client*, MQTT_Packet*);
static void processPacketConnack(MQTT_Client*, MQTT_Packet*);


__weak void MQTT_PacketSend(MQTT_Client *packet, uint8_t *data, uint16_t size)
{
  return;
}


__weak void MQTT_PacketRecieve(MQTT_Client *packet)
{
  return;
}

__weak void MQTT_LockCMD(MQTT_Client *mqttClient)
{
  while(MQTT_IS_STATUS(mqttClient, MQTT_STAT_RUNNING_CMD));
  MQTT_SET_STATUS(mqttClient, MQTT_STAT_RUNNING_CMD);
}


__weak void MQTT_UnlockCMD(MQTT_Client *mqttClient)
{
  MQTT_UNSET_STATUS(mqttClient, MQTT_STAT_RUNNING_CMD);
}


__weak void MQTT_WaitResponse(MQTT_Client *mqttClient)
{
  while(!MQTT_IS_STATUS(mqttClient, MQTT_STAT_RECEIVE_IT));
  MQTT_UNSET_STATUS(mqttClient, MQTT_STAT_RECEIVE_IT);
}


void MQTT_SetAuth(MQTT_Client *mqttClient, const char *username, const char *password)
{
  uint16_t i;
  
  for (i = 0; i < MQTT_USERNAME_SZ_MAX-1; i++)
  {
    mqttClient->auth.username[i] = *username;
    username++;
    if(!mqttClient->auth.username[i]) break;
  }
  for (i = 0; i < MQTT_PASSWORD_SZ_MAX-1; i++)
  {
    mqttClient->auth.password[i] = *password;
    password++;
    if(!mqttClient->auth.password[i]) break;
  }
  
}


void MQTT_Connect(MQTT_Client *mqttClient, const char *id)
{
  MQTT_Packet packet;
  MQTT_Packet respPacket;
  uint8_t *packetBuffer;
  uint8_t cFlag;

  MQTT_LockCMD(mqttClient);

  cFlag = 2; // clean set
  if(mqttClient->auth.username[0] != 0) cFlag |= 0x80;
  if(mqttClient->auth.password[0] != 0) cFlag |= 0x40;

  packet = MQTT_Packet_New(MQTT_PACKET_TYPE_CONNECT, mqttClient->txBuffer);
  MQTT_Packet_WriteBytes(&packet, (uint8_t *) "MQTT", 4);
  MQTT_Packet_WriteInt8(&packet, MQTT_PROTOCOL_VERSION);
  MQTT_Packet_WriteInt8(&packet, cFlag);
  MQTT_Packet_WriteInt16(&packet, mqttClient->options.keepAlive);

  // Properties
  MQTT_Packet_StartWriteProperties(&packet);
  {
    if(mqttClient->options.sessionExpInterval){
      MQTT_Packet_WriteProperties(&packet, 
        MQTT_PROP_SESS_EXP_INTV, 
        (uint8_t *) &(mqttClient->options.sessionExpInterval), 1
      );
    }
  }
  MQTT_Packet_StopWriteProperties(&packet);
  
  // Payload
  MQTT_Packet_WriteBytes(&packet, (const uint8_t *) id, strlen(id));

  if(mqttClient->auth.username[0] != 0){
    MQTT_Packet_WriteBytes(&packet,
      (uint8_t *) mqttClient->auth.username,
      strlen(mqttClient->auth.username)
    );
  }

  if(mqttClient->auth.password[0] != 0){
    MQTT_Packet_WriteBytes(&packet,
      (uint8_t *) mqttClient->auth.password,
      strlen(mqttClient->auth.password)
    );
  }

  packetBuffer = MQTT_Packet_Encode(&packet);
  MQTT_PacketSend(mqttClient, packetBuffer, packet.bufferLen);
  MQTT_WaitResponse(mqttClient);
  respPacket = MQTT_Packet_Decode(mqttClient->rxBuffer, mqttClient->rxBufferLen);
  MQTT_UnlockCMD(mqttClient);
}


void MQTT_Publish(MQTT_Client *mqttClient, const char *topic, uint8_t QoS)
{
  MQTT_Packet packet;
  uint8_t *packetBuffer;

  MQTT_LockCMD(mqttClient);

  packet = MQTT_Packet_New(MQTT_PACKET_TYPE_PUBLISH, mqttClient->txBuffer);
  packet.type |= (QoS & 0x03) << 1;

  /**
   * Variable Header
   * 1. topic
   * 2. packet ID
   * 3. propperties
   */
  MQTT_Packet_WriteBytes(&packet, (const uint8_t *) topic, strlen(topic));



  packetBuffer = MQTT_Packet_Encode(&packet);
  MQTT_PacketSend(mqttClient, packetBuffer, packet.bufferLen);
  MQTT_UnlockCMD(mqttClient);
}


static void processPacket(MQTT_Client *mqttClient, MQTT_Packet *packet)
{
  MQTT_PacketType pType;

  pType = packet->type & 0xF0;

  switch (pType)
  {
  case MQTT_PACKET_TYPE_CONNACK:
    processPacketConnack(mqttClient, packet);
    break;
  
  default:
    break;
  }
}


static void processPacketConnack(MQTT_Client *mqttClient, MQTT_Packet *packet)
{
  
}
