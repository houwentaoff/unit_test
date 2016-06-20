/*******************************************************************************
 * CommanMain.c
 *
 * Histroy:
 *  2011-9-26 2011 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include "QtAmfCompat.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
int flag;
int needstop;
enum CameraState cameraStatus;
static void sigstop(int sig)
{
  (void)sig;
  fprintf(stderr, "Received signal interrupt!\n");
  switch(cameraStatus) {
    case CAM_STATE_REC_RECORDING: {
      fprintf(stderr, "Stop recording!\n");
      record_stop();
      flag = 0;
      needstop = 0;
    }break;
    case CAM_STATE_PB_PLAYING: {
      fprintf(stderr, "Stop playing!\n");
      playback_stop();
      flag = 0;
      needstop = 0;
    }break;
    default:
      break;
  }
}

static void recordMsgCallback(int state)
{
  fprintf(stderr, "recordMsgCallback is called!\n");
  cameraStatus = (enum CameraState)state;
  switch(cameraStatus) {
    case CAM_STATE_REC_STOPPING:
    case CAM_STATE_REC_STOPPED: {
      fprintf(stderr, "Rcording stopped!\n");
      needstop = 0;
      flag = 0;
    }break;
    case CAM_STATE_REC_RECORDING: {
      fprintf(stderr, "Recording started!\n");
    }break;
    case CAM_STATE_REC_ERROR: {
      fprintf(stderr, "Error occurs during recording!\n");
    }break;
    case CAM_STATE_REC_STOP_FAILED: {
      fprintf(stderr, "Failed to stop recording!\n");
      /*Need to notify program to stop recording*/
      flag = 0;
      needstop = 1;
    }break;
    default: {
      fprintf(stderr, "Unknown recording status!\n");
    }break;
  }
}

static void playbackMsgCallback(int state)
{
  fprintf(stderr, "playbackMsgCallback is called!\n");
  cameraStatus = (enum CameraState)state;
  switch(cameraStatus) {
    case CAM_STATE_PB_STOPPED: {
      fprintf(stderr, "Playing stopped!\n");
      flag = 0;
      needstop = 0;
    }break;
    case CAM_STATE_PB_PLAYING: {
      fprintf(stderr, "Playing started!\n");
    }break;
    case CAM_STATE_PB_PAUSED: {
      fprintf(stderr, "Playing paused!\n");
    }break;
    case CAM_STATE_PB_STOP_FAILED: {
      fprintf(stderr, "Playing is over but failed to stop!\n");
      /*Need to notify program to stop playing*/
      flag = 0;
      needstop = 1;
    }break;
    default: {
      fprintf(stderr, "Unknown playback status!\n");
    }break;
  }
}

int main()
{
  int index = 0;
  char *filelist[64] = {0};
  int fileCounts = 0;
  signal(SIGINT,  sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);
  needstop = 0;
  cameraStatus = CAM_STATE_UNKNOWN;

  if (QAmf_malloc() < 0) {
    printf ("QAmf Malloc error!\n");
    exit (-1);
  }
  if (camera_start() < 0) {
    printf ("Camera Failed to start!\n");
    QAmf_delete();
    exit (-1);
  }
  printf ("Camera Start OK!\n");
  //Enter preview
  printf ("Enter preview: ");
  if (camera_enter_preview() < 0) {
    printf ("Failed!\n");
    QAmf_delete();
    exit (-1);
  } else {
    printf ("OK!\n");
  }
  sleep(2);

  //Goto Idle
  printf ("Goto Idle: ");
  if (camera_goto_idle() < 0) {
    printf ("Failed!\n");
    exit (-1);
  } else {
    printf ("OK!\n");
  }
  sleep(2);

  //Recording
  printf ("Recording for a while...\n");
  set_record_msg_callback(recordMsgCallback);
  if (record_start() < 0) {
    printf ("Recording failed!\n");
    QAmf_delete();
    exit (-1);
  }
  flag = 1;
  needstop = 1;
  int count = 0;
  while (flag && (count < 8)) {
    sleep(1);
    ++ count;
  }

  if (needstop && record_stop() < 0) {
    printf ("Recording stop failed!\n");
    QAmf_delete();
    exit (-1);
  }

  //Playback
  printf ("Playing file...");
  fileCounts = record_get_file_list(filelist, 64);
  if (fileCounts > 0) {
    int ret = -1;
    int count = 0;
    printf ("file number is %d\n", fileCounts);
    printf ("playing %s\n", filelist[0]);
    set_playback_msg_callback(playbackMsgCallback);
    ret = playback_start(filelist[0]);
    for (count = 0; count < fileCounts; ++ count) {
      free(filelist[count]);
    }
    if (ret < 0) {
      printf ("Playing failed!\n");
      QAmf_delete();
      exit (-1);
    }
    flag = 1;
    needstop = 0;
    while (flag) {
      sleep(2);
    }
    if (needstop) {
      if (playback_stop() < 0) {
        printf ("Stop playing failed!\n");
        QAmf_delete();
        exit (-1);
      }
    }
  } else {
    printf("file number is %d\n", fileCounts);
    QAmf_delete();
    exit (-1);
  }

  //Playback exit test
  for (index = 0; index < 3; ++ index) {
    fileCounts = record_get_file_list(filelist, 64);
    if (fileCounts > 0) {
      int ret = -1;
      int count = 0;
      printf ("Playing file %s...\n", filelist[0]);
      ret = playback_start(filelist[0]);
      for (count = 0; count < fileCounts; ++ count) {
        free(filelist[count]);
      }
      if (ret < 0) {
        printf("Playing failed!\n");
        QAmf_delete();
        exit(-1);
      }
      sleep(2);
      printf ("Pause for 2 seconds...\n");
      if (playback_pause() < 0) {
        printf("Pause failed!\n");
        QAmf_delete();
        exit(-1);
      }
      sleep(2);
      printf ("Resume playing...\n");
      if (playback_resume() < 0) {
        printf("Resume failed!\n");
        QAmf_delete();
        exit(-1);
      }
      printf ("Pause again...\n");
      if (playback_pause() < 0) {
        printf("Pause failed!\n");
        QAmf_delete();
        exit(-1);
      }
      sleep(1);
      if (playback_stop() < 0) {
        printf ("Stop playing after pause failed!\n");
        if (camera_goto_idle() < 0) {
          printf("Goto Idle failed!\n");
          QAmf_delete();
          exit (-1);
        }
      }
    }
    sleep(1); //Wait for Audio device to become ready
  }

  printf ("Test LCD...\n");
  //Flip video
  printf ("Flip Video: ");
  for (index = 3; index >= 0; -- index) {
    if (lcd_flip_video(index) < 0) {
      printf ("Failed!\n");
      QAmf_delete();
      exit (-1);
    }
    if (camera_enter_preview() < 0) {
      printf ("Enter preview failed!\n");
      QAmf_delete();
      exit (-1);
    }
    sleep(3);
  }
  printf ("OK!\n");

  sleep(2);
  if (lcd_set_video_offset(0, 0, 1) < 0) {
    printf ("Failed!\n");
    QAmf_delete();
    exit (-1);
  }
  printf ("OK!\n");

  //Change Video Size
  printf ("Change Video size: ");
  if (lcd_change_video_size(240, 160) < 0) {
    printf ("Failed!\n");
    QAmf_delete();
    exit (-1);
  }
  if (camera_enter_preview() < 0) {
    printf ("Enter preview failed!\n");
    QAmf_delete();
    exit (-1);
  }
  sleep(2);

  //Change Video Offset
  printf ("Set video offset: ");
  if (lcd_set_video_offset(80, 80, 0) < 0) {
    printf ("Failed!\n");
    QAmf_delete();
    exit (-1);
  }
  if (camera_enter_preview() < 0) {
    printf ("Enter preview failed!\n");
    QAmf_delete();
    exit (-1);
  }
  sleep(2);

  if (lcd_set_video_offset(0, 0, 1) < 0) {
    printf ("Failed!\n");
    QAmf_delete();
    exit (-1);
  }
  if (camera_enter_preview() < 0) {
    printf ("Enter preview failed!\n");
    QAmf_delete();
    exit (-1);
  }
  sleep(2);

  if (lcd_set_video_offset(0, 0, 0) < 0) {
    printf ("Failed!\n");
    QAmf_delete();
    exit (-1);
  }
  if (camera_enter_preview() < 0) {
    printf ("Enter preview failed!\n");
    QAmf_delete();
    exit (-1);
  }
  sleep(2);

  //Change Video Size
  printf ("LCD resolution is %dx%d\n",lcd_width(), lcd_height());
  if (lcd_change_video_size(lcd_width(), lcd_height()) < 0) {
    printf ("Failed!\n");
    QAmf_delete();
    exit (-1);
  }
  if (camera_enter_preview() < 0) {
    printf ("Enter preview failed!\n");
    QAmf_delete();
    exit (-1);
  }
  sleep(2);

  printf ("Reset LCD: ");
  if (lcd_reset() < 0) {
    printf ("Failed!\n");
    QAmf_delete();
    exit (-1);
  }
  printf ("OK!\n");

  printf ("Stop HDMI: ");
  if (hdmi_stop() < 0) {
    printf ("Failed!\n");
    QAmf_delete();
    exit(-1);
  }
  printf ("OK!\n");
  printf ("Reset HDMI: ");
  if (hdmi_reset() < 0) {
    printf ("Failed!\n");
    QAmf_delete();
    exit (-1);
  }
  printf ("OK!\n");
  if (camera_enter_preview() < 0) {
    printf ("Enter preview failed!\n");
    QAmf_delete();
    exit (-1);
  }
  sleep(2);

  printf ("Test done!\n");
  QAmf_delete();
  printf ("QAmf delete...\n");
  return 0;
}
