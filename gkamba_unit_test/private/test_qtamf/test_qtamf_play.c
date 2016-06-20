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
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

enum CameraState cameraStatus;
int flag;
int needstop;
static void sigstop(int sig)
{
  (void)sig;
  fprintf(stderr, "Received signal interrupt!\n");
  switch(cameraStatus) {
    case CAM_STATE_PB_PAUSED:
    case CAM_STATE_PB_PLAYING: {
      fprintf(stderr, "Stop Playing!\n");
      playback_stop();
      flag = 0;
    }break;
    default: {
      flag = 0;
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

void runIdle()
{
  char ch = 0;
  int flags = fcntl(STDIN_FILENO, F_GETFL);

  flags |= O_NONBLOCK;
  flags = fcntl(STDIN_FILENO, F_SETFL, flags);
  while (flag) {
    if (read(STDIN_FILENO, &ch, 1) < 0) {
      usleep (100000);
      continue;
    }
    switch (ch) {
      case 'P':
      case 'p': {
        if (playback_pause() < 0) {
          fprintf(stderr, "Pause failed!\n");
        }
      }break;
      case 'R':
      case 'r': {
        if (playback_resume() < 0) {
          fprintf(stderr, "Resume failed!\n");
        }
      }break;
      case 'Q':
      case 'q':
      case 'S':
      case 's': {
        sigstop(ch);
      }break;
      default: break;
    }
  }
}

int main(int argc, char * argv[])
{
  if (argc != 2) {
    fprintf (stderr, "Usage: test_playback file_to_play.[ts|mp4|pcm]\n");
    exit(-1);
  }
  signal(SIGINT,  sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);
  flag = 1;
  cameraStatus = CAM_STATE_UNKNOWN;

  if (QAmf_malloc() < 0) {
    fprintf (stderr, "QAmf Malloc error!\n");
    exit (-1);
  }
  if (camera_start() < 0) {
    fprintf (stderr, "Camera Failed to start!\n");
    QAmf_delete();
    exit (-1);
  }
  fprintf (stderr, "Camera Start OK!\n");

#if 0
  //Enter preview
  fprintf (stderr, "Enter preview: ");
  if (camera_enter_preview() < 0) {
    fprintf (stderr, "Failed!\n");
    QAmf_delete();
    exit (-1);
  } else {
    fprintf (stderr, "OK!\n");
  }
  fprintf (stderr, "Stay in preivew for 60 seconds!\n");
  int count = 0;
  while (count < 60) {
    ++ count;
    sleep(1);
    fprintf (stderr, ".");
  }
  fprintf (stderr, "\n");
#endif

  //Goto Idle
  fprintf (stderr, "Goto Idle: ");
  if (camera_goto_idle() < 0) {
    fprintf (stderr, "Failed!\n");
    exit (-1);
  } else {
    fprintf (stderr, "OK!\n");
  }

  //Playback
  fprintf (stderr, "Playing file...");
  set_playback_msg_callback(playbackMsgCallback);
  if (playback_start(argv[1]) < 0) {
    fprintf (stderr, "Playing failed!\n");
    QAmf_delete();
    exit (-1);
  }
  runIdle();
  if (needstop) {
    if (playback_stop() < 0) {
      printf ("Stop playing failed!\n");
      QAmf_delete();
      exit (-1);
    }
  }

  fprintf (stderr, "Test done!\n");
  QAmf_delete();
  fprintf (stderr, "QAmf delete...\n");
  return 0;
}

