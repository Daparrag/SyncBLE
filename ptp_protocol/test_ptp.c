/*this file brings suport for the packet creation/parse for the ptp protocol*/
/*Test file for ptp protocol*/
#include "test_ptp.h"

 uint8_t test_create_header(uint8_t type, ptp_hdr * hdr,uint8_t * buff){
 	/*This test consist in the creation of a simple ptp_header 
	 * If everything is ok this function return a value !=0 otherwise 0
 	*/
 	uint8_t ret;
 	ret = create_ptp_packet_hdr(0,type,hdr,buff);

 	return ret;
 }


 uint8_t test_parse_header(ptp_hdr * hdr,uint8_t * data, uint8_t datalen){
 	/*This test consist parse a simple ptp_header  
	 * If everything is ok this function return a value !=0 otherwise 0
 	*/
 	uint8_t ret;
 	ret = ptp_packet_hdr_parse(data,datalen, hdr);
 	return ret;

 }

 uint8_t compare_headers(ptp_hdr * s1hdr,ptp_hdr * s2hdr){
 	/*This test consist compare two header data structures   
	 * If everything is ok this function return a value !=0 otherwise 0
 	*/
 	uint8_t ret=1;

 	if(s1hdr->ptp_type!=s1hdr->ptp_type)ret=0;
 	if(s1hdr->ptp_version!=s1hdr->ptp_version)ret=0;
 	if(s1hdr->domain_number!=s1hdr->domain_number)ret=0;
 	if(s1hdr->domain_number!=s1hdr->domain_number)ret=0;
 	if(s1hdr->control_field!=s1hdr->control_field)ret=0;
 	if(s1hdr->sequence_id!=s1hdr->sequence_id)ret=0;
 	if(s1hdr->msg_sync_interval!=s1hdr->msg_sync_interval)ret=0;
 	if(s1hdr->source_id!=s1hdr->source_id)ret=0;

 	return ret;


 }

 uint8_t test_header(){
 	/* This test is used to create and parse a ptp_header and
 	 * compare the results
 	 * * If everything is ok this function return a value !=0 otherwise 0
 	 */
uint8_t ret;
uint8_t tst_buff [5]; /*buffer set with a size of the header*/
ptp_hdr tst_ptphdr_cr; /*used to create the header */
ptp_hdr tst_ptphdr_pr;	/*used to parce the header */
 	/*1. create a sync_header*/
		ret = test_create_header(SYNC,&tst_ptphdr_cr,tst_buff);
		if (ret==0)return ret;
		ret=test_parse_header(&tst_ptphdr_pr,tst_buff,ret);
		if(ret==0)return ret;
		ret=compare_headers(&tst_ptphdr_cr,&tst_ptphdr_pr);
		if(ret==0)return ret;
return ret;		

 }