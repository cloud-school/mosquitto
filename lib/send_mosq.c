/*
Copyright (c) 2009,2010, Roger Light <roger@atchoo.org>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of mosquitto nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdint.h>
#include <string.h>

#include <mosquitto.h>
#include <mqtt3_protocol.h>
#include <net_mosq.h>

/* For PUBACK, PUBCOMP, PUBREC, and PUBREL */
int _mosquitto_send_command_with_mid(struct mosquitto *mosq, uint8_t command, uint16_t mid)
{
	struct _mosquitto_packet *packet = NULL;

	packet = calloc(1, sizeof(struct _mosquitto_packet));
	if(!packet) return 1;

	packet->command = command;
	packet->remaining_length = 2;
	packet->payload = malloc(sizeof(uint8_t)*2);
	if(!packet->payload){
		free(packet);
		return 1;
	}
	packet->payload[0] = MOSQ_MSB(mid);
	packet->payload[1] = MOSQ_LSB(mid);

	if(_mosquitto_packet_queue(mosq, packet)) return 1;

	return 0;
}

/* For DISCONNECT, PINGREQ and PINGRESP */
int _mosquitto_send_simple_command(struct mosquitto *mosq, uint8_t command)
{
	struct _mosquitto_packet *packet = NULL;

	packet = calloc(1, sizeof(struct _mosquitto_packet));
	if(!packet) return 1;

	packet->command = command;
	packet->remaining_length = 0;

	if(_mosquitto_packet_queue(mosq, packet)){
		free(packet);
		return 1;
	}

	return 0;
}

int _mosquitto_send_pingreq(struct mosquitto *mosq)
{
	// FIXME if(mosq) _mosquitto_log_printf(MQTT3_LOG_DEBUG, "Sending PINGREQ to %s", mosq->id);
	return _mosquitto_send_simple_command(mosq, PINGREQ);
}

int _mosquitto_send_pingresp(struct mosquitto *mosq)
{
	// FIXME if(mosq) _mosquitto_log_printf(MQTT3_LOG_DEBUG, "Sending PINGRESP to %s", mosq->id);
	return _mosquitto_send_simple_command(mosq, PINGRESP);
}

int _mosquitto_send_puback(struct mosquitto *mosq, uint16_t mid)
{
	// FIXME if(mosq) mqtt3_log_printf(MQTT3_LOG_DEBUG, "Sending PUBACK to %s (Mid: %d)", mosq->id, mid);
	return _mosquitto_send_command_with_mid(mosq, PUBACK, mid);
}

int _mosquitto_send_publish(struct mosquitto *mosq, const char *topic, uint32_t payloadlen, const uint8_t *payload, int qos, bool retain, bool dup)
{
	struct _mosquitto_packet *packet = NULL;
	int packetlen;
	uint16_t mid = 1; //FIXME

	if(!mosq || mosq->sock == -1 || !topic) return 1;

	//if(mosq) _mosquitto_log_printf(MQTT3_LOG_DEBUG, "Sending PUBLISH to %s (%d, %d, %d, %d, '%s', ... (%ld bytes))", mosq->id, dup, qos, retain, mid, topic, (long)payloadlen);

	packetlen = 2+strlen(topic) + payloadlen;
	if(qos > 0) packetlen += 2; /* For message id */
	packet = calloc(1, sizeof(struct _mosquitto_packet));
	if(!packet){
		//_mosquitto_log_printf(MQTT3_LOG_DEBUG, "PUBLISH failed allocating packet memory.");
		return 1;
	}

	packet->command = PUBLISH | ((dup&0x1)<<3) | (qos<<1) | retain;
	packet->remaining_length = packetlen;
	packet->payload = malloc(sizeof(uint8_t)*packetlen);
	if(!packet->payload){
		//_mosquitto_log_printf(MQTT3_LOG_DEBUG, "PUBLISH failed allocating payload memory.");
		free(packet);
		return 1;
	}
	/* Variable header (topic string) */
	if(_mosquitto_write_string(packet, topic, strlen(topic))){
		//_mosquitto_log_printf(MQTT3_LOG_DEBUG, "PUBLISH failed writing topic.");
		free(packet);
	  	return 1;
	}
	if(qos > 0){
		if(_mosquitto_write_uint16(packet, mid)){
			//_mosquitto_log_printf(MQTT3_LOG_DEBUG, "PUBLISH failed writing mid.");
			free(packet);
			return 1;
		}
	}

	/* Payload */
	if(payloadlen && _mosquitto_write_bytes(packet, payload, payloadlen)){
		//_mosquitto_log_printf(MQTT3_LOG_DEBUG, "PUBLISH failed writing payload.");
		free(packet);
		return 1;
	}

	if(_mosquitto_packet_queue(mosq, packet)){
		//_mosquitto_log_printf(MQTT3_LOG_DEBUG, "PUBLISH failed queuing packet.");
		free(packet);
		return 1;
	}

	return 0;
}

int _mosquitto_send_pubrec(struct mosquitto *mosq, uint16_t mid)
{
	// FIXME if(mosq) _mosquitto_log_printf(MQTT3_LOG_DEBUG, "Sending PUBREC to %s (Mid: %d)", mosq->id, mid);
	return _mosquitto_send_command_with_mid(mosq, PUBREC, mid);
}

int _mosquitto_send_pubrel(struct mosquitto *mosq, uint16_t mid)
{
	// FIXME if(mosq) _mosquitto_log_printf(MQTT3_LOG_DEBUG, "Sending PUBREL to %s (Mid: %d)", mosq->id, mid);
	return _mosquitto_send_command_with_mid(mosq, PUBREL, mid);
}
