# if USE_DXR3
# DXR3_DIR=dxr3
# else
# DXR3_DIR=
# endif

if USE_OSS
OSS_DIR=oss
else
OSS_DIR=
endif

if USE_DIRECTDRAW
DIRECTDRAW_DIR=directdraw
else
DIRECTDRAW_DIR=
endif

if USE_DIRECTSOUND
DIRECTSOUND_DIR=directsound
else
DIRECTSOUND_DIR=
endif

if USE_SUNAUDIO
SUNAUDIO_DIR=sunaudio
else
SUNAUDIO_DIR=
endif

if USE_OSX_AUDIO
OSX_AUDIO_DIR=osxaudio
else
OSX_AUDIO_DIR=
endif

if USE_OSX_VIDEO
OSX_VIDEO_DIR=osxvideo
else
OSX_VIDEO_DIR=
endif

# if USE_QCAM
# QCAM_DIR=qcam
# else
# QCAM_DIR=
# endif

if USE_GST_V4L2
V4L2_DIR=v4l2
else
V4L2_DIR=
endif

# if USE_VCD
# VCD_DIR=vcd
# else
# VCD_DIR=
# endif

# if USE_CDROM
#  CDROM_DIR=cdrom
# else
# CDROM_DIR=
# endif

# if USE_OPENGL
# GL_DIR=glsink
# else
# GL_DIR=
# endif

if USE_X
XIMAGE_DIR=ximage
else
XIMAGE_DIR=
endif

SUBDIRS=$(DIRECTDRAW_DIR) $(DIRECTSOUND_DIR) $(OSS_DIR) $(OSX_AUDIO_DIR) $(OSX_VIDEO_DIR) $(SUNAUDIO_DIR) $(V4L2_DIR) $(XIMAGE_DIR)

DIST_SUBDIRS=directdraw directsound oss osxaudio osxvideo sunaudio v4l2 waveform ximage
