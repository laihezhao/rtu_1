#ifndef __PACKET_H__
#define __PACKET_H__


int get_sys_time(const char *, const char *);
void parse_time(char *, const char *);
void reverse_time(char *, const char *);

int create_pkt_alarm_data(char *, const char *);
int create_pkt_sbs_change(char *, const char *);
int create_pkt_sbs_status(char *, const char *);

int create_pkt_login(char *, const char *);
int create_pkt_heartbeat(char *, const char *);
int create_pkt_sampling_freq(char *, const char *);
int create_pkt_gettime(char *, const char *);
int create_pkt_exec(char *, const char *);
int create_pkt_response(char *, const char *);
int create_pkt_getlevel(char *, const char *);
int create_pkt_realtime_data(char *, const char *);
int create_pkt_minute_data(char *, const char *);
int create_pkt_hourly_data(char *, const char *);
int create_pkt_daily_data(char *, const char *);
int verify_packet(const char *);
int stop_pkt_realtime_data(char *);
int start_pkt_realtime_data(char *);

#endif //__PACKET_H_
