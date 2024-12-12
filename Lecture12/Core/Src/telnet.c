/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */


#include "lwip/opt.h"
#include <string.h>
#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"

#define TELNET_THREAD_PRIO  ( tskIDLE_PRIORITY + 4 )
#define CMD_BUFFER_LEN 255

static void http_client(char *s, uint16_t size){
	struct netconn *client;
	struct netbuf *buf;
	ip_addr_t ip;
	uint16_t len = 0;
	IP_ADDR4(&ip, 147,229,144,124);
	const char *request = "GET /ip.php HTTP/1.1\r\n"
			"Host: www.urel.feec.vutbr.cz\r\n"
			"Connection: close\r\n"
			"\r\n\r\n";
	client = netconn_new(NETCONN_TCP);
	if (netconn_connect(client, &ip, 80) == ERR_OK) {
		netconn_write(client, request, strlen(request), NETCONN_COPY);
		// Receive the HTTP response
		s[0] = 0;
		while (len < size && netconn_recv(client, &buf) == ERR_OK) {
			len += netbuf_copy(buf, &s[len], size-len);
			s[len] = 0;
			netbuf_delete(buf);
		}
	} else {
		sprintf(s, "Connection error\n");
	}
	netconn_delete(client);
}

static void telnet_process_command(char *cmd, struct netconn *conn){
	char command[CMD_BUFFER_LEN];
	sprintf(command, "%s", cmd); // Copy the incoming command to a new string

	if (strcmp(command, "HELLO") == 0) {
	    netconn_write(conn, "COMMUNICATION IS OK\n", strlen("COMMUNICATION IS OK\n"), NETCONN_COPY);
	} else if (strcmp(command, "LED1 ON") == 0) {
		HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);
	    netconn_write(conn, "LED1 is ON\n", strlen("LED1 is ON\n"), NETCONN_COPY);
	} else if (strcmp(command, "LED1 OFF") == 0) {
		HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
	    netconn_write(conn, "LED1 is OFF\n", strlen("LED1 is OFF\n"), NETCONN_COPY);
	} else if (strcmp(command, "LED2 ON") == 0) {
		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
	    netconn_write(conn, "LED2 is ON\n", strlen("LED2 is ON\n"), NETCONN_COPY);
	} else if (strcmp(command, "LED2 OFF") == 0) {
	    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
	    netconn_write(conn, "LED2 is OFF\n", strlen("LED2 is OFF\n"), NETCONN_COPY);
	} else if (strcmp(command, "LED3 ON") == 0) {
	    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
	    netconn_write(conn, "LED3 is ON\n", strlen("LED3 is ON\n"), NETCONN_COPY);
	} else if (strcmp(command, "LED3 OFF") == 0) {
	    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
	    netconn_write(conn, "LED3 is OFF\n", strlen("LED3 is OFF\n"), NETCONN_COPY);
	} else if (strcmp(command, "STATUS") == 0) {
	    uint32_t ld1_state = HAL_GPIO_ReadPin(LD1_GPIO_Port, LD1_Pin);
	    uint32_t ld2_state = HAL_GPIO_ReadPin(LD2_GPIO_Port, LD2_Pin);
	    uint32_t ld3_state = HAL_GPIO_ReadPin(LD3_GPIO_Port, LD3_Pin);
	    char status[50];
	    sprintf(status, "LED1: %s, LED2: %s, LED3: %s\n", (ld1_state == GPIO_PIN_SET) ? "ON" : "OFF", (ld2_state == GPIO_PIN_SET) ? "ON" : "OFF", (ld3_state == GPIO_PIN_SET) ? "ON" : "OFF");
	    netconn_write(conn, status, strlen(status), NETCONN_COPY);
	} else if (strcmp(command, "CLIENT") == 0){
		char s[1024];
		http_client(s,1024);
		netconn_write(conn, s, strlen(s), NETCONN_COPY);
	}
}

static void telnet_byte_available(uint8_t c, struct netconn *conn)
{
 static uint16_t cnt;
 static char data[CMD_BUFFER_LEN];
 if (cnt < CMD_BUFFER_LEN && c >= 32 && c <= 127) data[cnt++] = c;
 if (c == '\n' || c == '\r') {
 data[cnt] = '\0';
 telnet_process_command(data, conn);
 cnt = 0;
 }
}

/*-----------------------------------------------------------------------------------*/
static void telnet_thread(void *arg)
{
  struct netconn *conn, *newconn;
  err_t err, accept_err;
  struct netbuf *buf;
  uint8_t *data;
  u16_t len;
      
  LWIP_UNUSED_ARG(arg);

  /* Create a new connection identifier. */
  conn = netconn_new(NETCONN_TCP);
  
  if (conn!=NULL)
  {  
    /* Bind connection to well known port number 23. */
    err = netconn_bind(conn, NULL, 23);
    
    if (err == ERR_OK)
    {
      /* Tell connection to go into listening mode. */
      netconn_listen(conn);
    
      while (1) 
      {
        /* Grab new connection. */
         accept_err = netconn_accept(conn, &newconn);
    
        /* Process the new connection. */
        if (accept_err == ERR_OK) 
        {

          while (netconn_recv(newconn, &buf) == ERR_OK) 
          {
            do 
            {
            	netbuf_data(buf, (void**)&data, &len);
            	while (len--) {telnet_byte_available(*data++, newconn);}
            } 
            while (netbuf_next(buf) >= 0);
          
            netbuf_delete(buf);
          }
        
          /* Close connection and discard connection identifier. */
          netconn_close(newconn);
          netconn_delete(newconn);
        }
      }
    }
    else
    {
      netconn_delete(newconn);
    }
  }
}

/*-----------------------------------------------------------------------------------*/

void telnet_init(void)
{
  sys_thread_new("telnet_thread", telnet_thread, NULL, DEFAULT_THREAD_STACKSIZE, TELNET_THREAD_PRIO);
}
/*-----------------------------------------------------------------------------------*/

#endif /* LWIP_NETCONN */
