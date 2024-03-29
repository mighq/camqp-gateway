	/* Linealize PDU to buffer ********************************************/
	memset(local_buffer, 0, sizeof(local_buffer));
	ret = smpp34_pack( req.command_id, (uint8_t*) local_buffer, sizeof(local_buffer),
			&local_buffer_len, (void*)&req);
	if( ret != 0 ){ printf("Error in smpp34_pack():%d:\n%s\n",
			smpp34_errno, smpp34_strerror); return FALSE; };
	/* Print PDU **********************************************************/
/*	memset(print_buffer, 0, sizeof(print_buffer));
	ret = smpp34_dumpPdu( req.command_id, print_buffer, 
			sizeof(print_buffer), (void*)&req);
	if( ret != 0){ printf("Error in smpp34_dumpPdu():%d:\n%s\n",
			smpp34_errno, smpp34_strerror); return; };
	printf("-----------------------------------------------------------\n");
	printf("SENDING PDU \n%s\n", print_buffer);*/
	/* Print Buffer *******************************************************/
/*	memset(print_buffer, 0, sizeof(print_buffer));
	ret = smpp34_dumpBuf(print_buffer, sizeof(print_buffer), 
			local_buffer, local_buffer_len);
	if( ret != 0 ){ printf("Error in smpp34_dumpBuf():%d:\n%s\n",
			smpp34_errno, smpp34_strerror ); return; };
	printf("SENDING BUFFER \n%s\n", print_buffer);*/
	/* Write to socket ****************************************************/
	ret = send(g_socket, local_buffer, local_buffer_len, 0); 
	if( ret != local_buffer_len ){ printf("Error in send()\n");return FALSE;};
