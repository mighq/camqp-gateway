	memset(local_buffer, 0, sizeof(local_buffer));
	/* Read from socket (This is a sample, must be more complex) **********/
	ret = recv(g_socket, local_buffer, 4, MSG_PEEK); 
	if( ret != 4 ){ printf("Error in recv(PEEK)\n");return FALSE;};
	memcpy(&tempo, local_buffer, sizeof(uint32_t)); /* get lenght PDU */
	local_buffer_len = ntohl( tempo );
	ret = recv(g_socket, (uint8_t*) local_buffer, local_buffer_len, 0); 
	if( ret != local_buffer_len ){ 
		printf("Error in recv(%d bytes)\n", local_buffer_len);return FALSE;};
	/* Print Buffer *******************************************************/
/*	memset(print_buffer, 0, sizeof(print_buffer));
	ret = smpp34_dumpBuf(print_buffer, sizeof(print_buffer), 
			local_buffer, local_buffer_len);
	if( ret != 0 ){ printf("Error in smpp34_dumpBuf():%d:\n%s\n",
			smpp34_errno, smpp34_strerror ); return; };
	printf("-----------------------------------------------------------\n");
	printf("RECEIVE BUFFER \n%s\n", print_buffer);*/
	/* unpack PDU *********************************************************/
	memcpy(&tempo, local_buffer+4, sizeof(uint32_t)); /* get command_id PDU */
	cmd_id = ntohl( tempo );
	ret = smpp34_unpack(cmd_id, (void*)&res, (uint8_t*) local_buffer, local_buffer_len);
	if( ret != 0){ printf( "Error in smpp34_unpack():%d:%s\n",
			smpp34_errno, smpp34_strerror); return FALSE; };
	/* Print PDU **********************************************************/
/*	memset(print_buffer, 0, sizeof(print_buffer));
	ret = smpp34_dumpPdu( res.command_id, print_buffer, 
			sizeof(print_buffer), (void*)&res);
	if( ret != 0){ printf("Error in smpp34_dumpPdu():%d:\n%s\n",
			smpp34_errno, smpp34_strerror); return; };
	printf("RECEIVE PDU \n%s\n", print_buffer);*/
