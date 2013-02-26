 #include <stdio.h>
 #include <stdlib.h>
 #include <vlc/vlc.h>
 
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
        
     /* Create a media player playing environement */
     mp = libvlc_media_player_new_from_media (m);
    
     /* No need to keep the media now */
     libvlc_media_release (m);
     
     /* play the media_player */
     libvlc_media_player_play (mp);
     printf("audio volume set: %d\n",libvlc_audio_set_volume(mp, 100));
 
    
     int i;
     for(i=0;i<20;i++) {
       int ms=libvlc_media_player_get_time(mp);
       printf("%02d:%02d.%03d\n",ms/1000/60,(ms/1000)%60,ms%1000);
       printf("audio volume set: %d\n",libvlc_audio_set_volume(mp, i*5));
       sleep(1);
     }
     
     libvlc_media_player_set_time(mp, (1*60+8)*1000+((100/75)*23)*10);

     sleep (10); /* Let it play a bit */

     /* Stop playing */
     libvlc_media_player_stop (mp);
 
     /* Free the media_player */
     libvlc_media_player_release (mp);
 
     libvlc_release (inst);
 
     return 0;
 }
