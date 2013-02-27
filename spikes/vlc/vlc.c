 #include <stdio.h>
 #include <stdlib.h>
 #include <vlc/vlc.h>
 
 void sleep_ms(int ms) {
   struct timespec ts = { 0, ((long) ms) * 1000000 };
   nanosleep(&ts, NULL);
 }
 
 static int stop = 0;
 
 void vlc_cb(const struct libvlc_event_t *e, void *data) {
   libvlc_media_player_t *mp = (libvlc_media_player_t *) data;
   printf("event: %d\n", e->type);
   int ms=libvlc_media_player_get_time(mp);
   printf("%02d:%02d.%03d\n",ms/1000/60,(ms/1000)%60,ms%1000);
   if (e->type == libvlc_MediaPlayerEndReached) {
     stop=1;
     printf("Stop time reached\n");
   }
 }
 
 int main(int argc, char* argv[])
 {
     libvlc_instance_t * inst;
     libvlc_media_player_t *mp;
     libvlc_media_t *m;
     
     /* Load the VLC engine */
     inst = libvlc_new (0, NULL);

     const char *media_file=argv[1];
     printf("Playing %s\n",media_file);
  
     /* Create a new item */
     m = libvlc_media_new_path (inst, media_file);
     char st[100], et[100];
     double start = 0.0;
     double end = 3*60 + 3 + 47/100.0;
     sprintf(st,"start-time=3.05%.lf", 47/75.0);
     sprintf(et,"stop-time=3.25%.lf",0.0);
     printf("%s, %s\n",st,et);
     //libvlc_media_add_option(m, st);
     libvlc_media_add_option(m, et);
        
     /* Create a media player playing environement */
     mp = libvlc_media_player_new_from_media (m);
    
     libvlc_event_manager_t *me = libvlc_media_player_event_manager(mp); 
     libvlc_event_attach(me, libvlc_MediaPlayerTimeChanged, vlc_cb, mp);
     libvlc_event_attach(me, libvlc_MediaPlayerPositionChanged, vlc_cb, mp);
     libvlc_event_attach(me, libvlc_MediaPlayerEndReached, vlc_cb, mp);
     
     /* play the media_player */
     
     libvlc_media_player_play (mp);
     libvlc_media_player_set_time(mp, (3*60+5)*1000+((100/75.0)*47)*10);
     printf("audio volume set: %d\n",libvlc_audio_set_volume(mp, 100));
 
     while(!stop) {
       sleep_ms(1);
     }
    
     /*int i;
     for(i=0;i<10000;i++) {
       int ms=libvlc_media_player_get_time(mp);
       printf("%02d:%02d.%03d\n",ms/1000/60,(ms/1000)%60,ms%1000);
       //printf("audio volume set: %d\n",libvlc_audio_set_volume(mp, i*5));
       sleep_ms(1);
     }*/
     
     //libvlc_media_player_set_time(mp, (1*60+8)*1000+((100/75)*23)*10);
     libvlc_media_release (m);
     m = libvlc_media_new_path (inst, media_file);
     sprintf(st,"start-time=%.3lf",end);
     sprintf(et,"stop-time=%.3lf",end+100);
     printf("%s, %s\n",st,et);

     libvlc_media_add_option(m, st);
     libvlc_media_add_option(m, et);
     libvlc_media_player_set_media(mp, m);
     //libvlc_media_player_set_time(mp, end*1000);
     libvlc_media_player_play (mp);
     

     sleep (10); /* Let it play a bit */

     /* No need to keep the media now */
     libvlc_media_release (m);
     

     /* Stop playing */
     libvlc_media_player_stop (mp);
 
     /* Free the media_player */
     libvlc_media_player_release (mp);
 
     libvlc_release (inst);
 
     return 0;
 }
