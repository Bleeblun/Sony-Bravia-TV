/*
* linux/sound/drivers/mtk/alsa_pcm.c
*
* MTK Sound Card Driver
*
* Copyright (c) 2010-2012 MediaTek Inc.
* $Author: dtvbm11 $
* 
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
* more details.
* http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
*
*/
 
/*
 *  mt85xx sound card based on streaming button sound
 */

#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/module.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/tlv.h>
#include <sound/pcm.h>
#include <sound/rawmidi.h>
#include <sound/initval.h>

//COMMON
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
typedef signed int INT32;
typedef bool BOOL;
//PLAYBACK
extern void AUD_InitALSAPlayback_MixSnd(UINT8 u1StreamId);
extern void AUD_DeInitALSAPlayback_MixSnd(UINT8 u1StreamId);
extern UINT32 AUD_GetMixSndFIFOStart(UINT8 u1StreamId);
extern UINT32 AUD_GetMixSndFIFOEnd(UINT8 u1StreamId);
extern UINT32 AUD_GetMixSndReadPtr(UINT8 u1StreamId);
extern void AUD_SetMixSndWritePtr(UINT8 u1StreamId, UINT32 u4WritePtr);
extern void AUD_PlayMixSndRingFifo(UINT8 u1StreamId, UINT32 u4SampleRate, UINT8 u1StereoOnOff, UINT8 u1BitDepth, UINT32 u4BufferSize);

#include "alsa_pcm.h"


#ifdef SUPPORT_COMPRESS_OFFLOAD
#include <uapi/sound/compress_params.h>
#include <sound/compress_offload.h>
#include <sound/compress_driver.h>
#include <linux/dma-mapping.h>

extern void* x_memcpy (void*        pv_to,
                       const void*  pv_from,
                       size_t       z_len);

extern void AUD_SetDSPDecoderType(AUD_FMT_T u4DspDecType);
extern void AUD_SetDSPSampleRate(SAMPLE_FREQ_T u4DspSamplingRate);
extern void AUD_SetDSPChannelNum(UINT8 u1DspChanNum);
extern void AUD_SetDSPDecoderPause(void);
extern void AUD_SetDSPDecoderResume(void);
extern void AUD_FlushAFIFO(void);
#endif
//////////////////////////////////////////

//#define SHOW_ALSA_LOG
#ifdef Printf
    #undef Printf
#endif    
#ifdef SHOW_ALSA_LOG
    #define Printf(fmt...)  printk(fmt)
#else
    #define Printf(fmt...)
#endif    

#define MAX_BUFFER_SIZE     (16 * 1024)
#define MAX_PERIOD_SIZE     (MAX_BUFFER_SIZE/2)
#define MIN_PERIOD_SIZE     (MAX_BUFFER_SIZE/16)

#define USE_FORMATS         (SNDRV_PCM_FMTBIT_S16_LE)  // DTV mixsnd engine supports 16bit, little-endian
#define USE_RATE            (SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_48000) //SNDRV_PCM_RATE_48000
#define USE_RATE_MIN        8000
#define USE_RATE_MAX        48000

#define USE_CHANNELS_MIN    1
#define USE_CHANNELS_MAX    2

#define USE_PERIODS_MIN     2
#define USE_PERIODS_MAX     16

#define ALSA_PCM_DECID 3  //for Compress Offload / Tunal PCM
#define HW_AV_SYNC 0x80


#ifdef SUPPORT_NEW_TUNAL_MODE
#define MAX_BUFFER_SIZE_AC3     (16 * 1024)
#define MAX_PERIOD_SIZE_AC3     (MAX_BUFFER_SIZE_AC3/2)
#define MIN_PERIOD_SIZE_AC3     (MAX_BUFFER_SIZE_AC3/16)

#define USE_PERIODS_MIN_AC3     2
#define USE_PERIODS_MAX_AC3     16
#define ALSA_DSPDEC_ID   3  // for Tunal PCM
#define ALSA_TUNAL_AC3_DSPDEC_ID   4  // for Tunal AC3
#define ALSA_NON_TUNAL_AC3_DSPDEC_ID   5  // for NON-Tunal AC3  
#endif

static int snd_card_mt85xx_pcm_playback_prepare(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct snd_mt85xx_pcm *pcm = runtime->private_data;

    int snd_pcm_device = substream->pcm->device;
    int snd_card_number = substream->pcm->card->number; 
    
    Printf("[ALSA] operator: prepare for card:%d, device:%d\n", snd_card_number, snd_pcm_device);

    pcm->pcm_buffer_size = snd_pcm_lib_buffer_bytes(substream);
    pcm->pcm_period_size = snd_pcm_lib_period_bytes(substream);
    pcm->pcm_rptr = 0;
    pcm->bytes_elapsed = 0;

    // Special workaround for USB camera pulg in USB 3.0, card0(mtk) will be chagned to card1
    if (snd_card_number==1)
    {
        Printf("[ALSA] Change snd_pcm_device from %d to %d for USB3.0 Camera plug-in\n", snd_pcm_device, snd_pcm_device-1);
        snd_pcm_device--;
            if (snd_pcm_device<0)
            {
                snd_pcm_device = 0;
                Printf("[ALSA] Convert device number error for Card1!!!\n");
            }
    }
    

    // Step 1: get button sound buffer parameters
    // the parameters are determined by audio driver
#ifndef MTK_AUDIO_SUPPORT_MULTI_STREAMOUT
    runtime->dma_area  = (unsigned char *)AUD_GetMixSndFIFOStart(substream->number);
#else
    runtime->dma_area  = (unsigned char *)AUD_GetMixSndFIFOStart(snd_pcm_device);
#endif
    runtime->dma_addr  = 0;
    runtime->dma_bytes = pcm->pcm_buffer_size;

//    pr_notice("[ALSA] pcm->pcm_buffer_size = %d (bytes)\n", pcm->pcm_buffer_size);
//    pr_notice("[ALSA] pcm->pcm_period_size = %d (bytes)\n", pcm->pcm_period_size);
//
//    pr_notice("[ALSA] runtime->dma_area  = 0x%X\n", (unsigned int)runtime->dma_area);
//    pr_notice("[ALSA] runtime->dma_bytes = 0x%X\n", runtime->dma_bytes);
//    pr_notice("[ALSA] runtime->rate      = %d\n", runtime->rate);
//    pr_notice("[ALSA] runtime->format    = %d (bitwidth = %d)\n", runtime->format, snd_pcm_format_width(runtime->format));
//    pr_notice("[ALSA] runtime->channels  = %d\n", runtime->channels);
//    pr_notice("[ALSA] runtime->delay     = %d (frames)\n", (int)runtime->delay);
//    pr_notice("[ALSA] runtime->start_threshold     = %d (frames)\n", (int)runtime->start_threshold);

#ifndef MTK_AUDIO_SUPPORT_MULTI_STREAMOUT 
    if (pcm->pcm_buffer_size > (AUD_GetMixSndFIFOEnd(substream->number) - AUD_GetMixSndFIFOStart(substream->number)))
#else
    if (pcm->pcm_buffer_size > (AUD_GetMixSndFIFOEnd(snd_pcm_device) - AUD_GetMixSndFIFOStart(snd_pcm_device)))
#endif
    {
        // buffer size must match
        return -EINVAL;
    }

    // init to silence
    snd_pcm_format_set_silence(runtime->format,
                               runtime->dma_area,
                               bytes_to_samples(runtime, runtime->dma_bytes));

    // setup button sound parameters
#ifndef MTK_AUDIO_SUPPORT_MULTI_STREAMOUT
    AUD_PlayMixSndRingFifo(substream->number, runtime->rate, (1 == runtime->channels) ? 0 : 1, runtime->sample_bits, pcm->pcm_buffer_size);
#else
    if (snd_pcm_device == ALSA_PCM_DECID)
    {
        //CC_AUD_AV_TUNNEL: HW_AV_SYNC is use SW_MIXSND3
       #ifndef SUPPORT_NEW_TUNAL_MODE_TEST //let  PORT_TUNAL_AC3=3, PORT_NON_TUNAL_AC3=3                                                               
        AUD_SetDSPDecoderType(AUD_FMT_PCM);
	   #else 
        AUD_SetDSPDecoderType(AUD_FMT_AC3);
	   #endif
    switch (runtime->rate /1000)
    {
    case 8: {
        AUD_SetDSPSampleRate(FS_8K);                                                      
        pr_notice("[ALSA PCM] SNDRV_PCM_RATE_8000\n");
        break;
    }   
    case 11: {
        AUD_SetDSPSampleRate(FS_11K);                                                                                 
        pr_notice("[ALSA PCM] SNDRV_PCM_RATE_11025\n");
        break;
    }
    case 16: {
        AUD_SetDSPSampleRate(FS_16K);                                                                                                               
        pr_notice("[ALSA PCM] SNDRV_PCM_RATE_16000\n");
        break;
    }
    case 22: {
        AUD_SetDSPSampleRate(FS_22K);                                                                                                                              
        pr_notice("[ALSA PCM] SNDRV_PCM_RATE_22050\n");
        break;
    }
    case 24: {
        AUD_SetDSPSampleRate(FS_24K);                                                                                                                              
        pr_notice("[ALSA PCM] SNDRV_PCM_RATE_24000\n");
        break;
    }
    case 32: {
        AUD_SetDSPSampleRate(FS_32K);                                                                                                                  
                                   
        pr_notice("[ALSA PCM] SNDRV_PCM_RATE_32000\n");
        break;
    }
    case 44: {
        AUD_SetDSPSampleRate(FS_44K);                                                                                                                  
                                           
        pr_notice("[ALSA PCM] SNDRV_PCM_RATE_44100\n");
        break;
    }
    case 48: {
        AUD_SetDSPSampleRate(FS_48K);                                                                                                                                  
                                        
        pr_notice("[ALSA PCM] SNDRV_PCM_RATE_48000\n");
        break;
    }
    case 64: {
        AUD_SetDSPSampleRate(FS_64K);                                                                                                                                                  
                                           
        pr_notice("[ALSA PCM] SNDRV_PCM_RATE_64000\n");
        break;
    }
    case 88: {
        AUD_SetDSPSampleRate(FS_88K);                                                                                                                                                  
                                          
        pr_notice("[ALSA PCM] SNDRV_PCM_RATE_88200\n");
        break;
    }
    case 96: {
        AUD_SetDSPSampleRate(FS_96K);                                                                                                                                                                  
                                         
        pr_notice("[ALSA PCM] SNDRV_PCM_RATE_96000\n");
        break;
    }
    case 176: {
        AUD_SetDSPSampleRate(FS_176K);                                                                                                                                                                                 
                                              
        pr_notice("[ALSA PCM] SNDRV_PCM_RATE_176400\n");
        break;
    }
    case 192: {
        AUD_SetDSPSampleRate(FS_192K);                                                                                                                                                                                                 
                                             
        pr_notice("[ALSA PCM] SNDRV_PCM_RATE_192000\n");
        break;
    }             
    default:
        pr_err("[ALSA PCM] !!!!codec not supported, sample_rate =%d\n", runtime->rate);
        return -EINVAL;
    }        
        AUD_PlayMixSndRingFifo(ALSA_PCM_DECID, runtime->rate, ((1 == runtime->channels) ? 0 : 1)|HW_AV_SYNC, runtime->sample_bits, pcm->pcm_buffer_size);
    }
#ifdef SUPPORT_NEW_TUNAL_MODE
    else if ((snd_pcm_device == ALSA_TUNAL_AC3_DSPDEC_ID) | (snd_pcm_device == ALSA_NON_TUNAL_AC3_DSPDEC_ID))
    {
      #if 0
       if((runtime->format == AUDIO_FORMAT_AC3)|(runtime->format == AUDIO_FORMAT_E_AC3))
       {
		   AUD_SetDSPDecoderType(AUD_FMT_AC3);
       } 	   
       else if ((runtime->format == AUDIO_FORMAT_AAC)|(runtime->format == AUDIO_FORMAT_HE_AAC_V1)|(runtime->format == AUDIO_FORMAT_HE_AAC_V2))
       {
		   AUD_SetDSPDecoderType(AUD_FMT_AAC);
       } 	   	
       else
       {
          
		  pr_err("[ALSA] !!!! codec not supported, id =%d\n",runtime->format);
		  return -EINVAL;
       }
	  #endif
        AUD_PlayMixSndRingFifo(snd_pcm_device, runtime->rate, ((1 == runtime->channels) ? 0 : 1)|HW_AV_SYNC, runtime->sample_bits, pcm->pcm_buffer_size);		
    }
#endif
    else
    {
        AUD_PlayMixSndRingFifo(snd_pcm_device, runtime->rate, (1 == runtime->channels) ? 0 : 1, runtime->sample_bits, pcm->pcm_buffer_size);
    }
#endif   
    return 0;
}

static void snd_card_mt85xx_pcm_playback_timer_start(struct snd_mt85xx_pcm *pcm)
{
    //2012/12/7 added by daniel
    if (pcm->timer_started)
    {
        del_timer_sync(&pcm->timer);
        pcm->timer_started = 0;
    }

    pcm->timer.expires = jiffies + pcm->pcm_period_size * HZ / (pcm->substream->runtime->rate * pcm->substream->runtime->channels * 2);
    add_timer(&pcm->timer);

    //2012/12/7 added by daniel
    pcm->timer_started = 1;
//    pr_notice("[ALSA] [%s], pcm->timer.expires=  %u, HZ = %d, jiffies = %u\n", __func__, (unsigned int)pcm->timer.expires, HZ, (unsigned int) jiffies);
//
//    pr_notice("[ALSA] [%s], pcm->pcm_period_size =  %u\n", __func__, pcm->pcm_period_size);
//    pr_notice("[ALSA] [%s], pcm->substream->runtime->rate =  %u\n", __func__, pcm->substream->runtime->rate);
//    pr_notice("[ALSA] [%s], pcm->substream->runtime->channels =  %u\n", __func__, pcm->substream->runtime->channels);


}

static void snd_card_mt85xx_pcm_playback_timer_stop(struct snd_mt85xx_pcm *pcm)
{
    if (del_timer(&pcm->timer)==1)
    {
        pcm->timer_started = 0;
    }
}

static void snd_card_mt85xx_pcm_playback_update_write_pointer(struct snd_mt85xx_pcm *pcm)
{
    struct snd_pcm_runtime *runtime = pcm->substream->runtime;
    unsigned int pcm_wptr = frames_to_bytes(runtime, runtime->control->appl_ptr % runtime->buffer_size);

    int snd_pcm_device = pcm->substream->pcm->device;
    int snd_card_number = pcm->substream->pcm->card->number; 
    // Special workaround for USB camera pulg in USB 3.0, card0(mtk) will be chagned to card1
    if (snd_card_number==1)
    {
        //Printf("[ALSA] Cahnge snd_pcm_device from %d to %d for USB3.0 Camera plug-in\n", snd_pcm_device, snd_pcm_device-1);
        snd_pcm_device--;
    }    

 #ifdef  ENABLE_ALSA_LOG 
    printk("[ALSA] @@@@@@@@@ ML-----snd_card_mt85xx_pcm_playback_update_write_pointer () @@@@@@@@ \n");
 #endif

    if (pcm_wptr == pcm->pcm_rptr)
    {
        // check if buffer full
        if (0 == snd_pcm_playback_avail(runtime)) {
#ifndef MTK_AUDIO_SUPPORT_MULTI_STREAMOUT 
            AUD_SetMixSndWritePtr(pcm->substream->number, AUD_GetMixSndFIFOStart(pcm->substream->number) + ((pcm->pcm_rptr + pcm->pcm_buffer_size - 1) % pcm->pcm_buffer_size));
#else
            AUD_SetMixSndWritePtr(snd_pcm_device, AUD_GetMixSndFIFOStart(snd_pcm_device) + ((pcm->pcm_rptr + pcm->pcm_buffer_size - 1) % pcm->pcm_buffer_size));
#endif
        } else {
          // no need process now, will be updated at next timer
          // Printf("[ALSA] timer: buffer empty\n");
        }
    }
    else
    {
#ifndef MTK_AUDIO_SUPPORT_MULTI_STREAMOUT
        AUD_SetMixSndWritePtr(pcm->substream->number, AUD_GetMixSndFIFOStart(pcm->substream->number) + pcm_wptr);
#else
        AUD_SetMixSndWritePtr(snd_pcm_device, AUD_GetMixSndFIFOStart(snd_pcm_device) + pcm_wptr);
#endif
    }
}

static int snd_card_mt85xx_pcm_playback_trigger(struct snd_pcm_substream *substream, int cmd)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct snd_mt85xx_pcm *pcm = runtime->private_data;
    int err = 0;

    Printf("[ALSA] operator: trigger, cmd = %d\n", cmd);

    spin_lock(&pcm->lock);
    switch (cmd) {
    case SNDRV_PCM_TRIGGER_START:
        snd_card_mt85xx_pcm_playback_update_write_pointer(pcm);

        snd_card_mt85xx_pcm_playback_timer_start(pcm);
        break;
    case SNDRV_PCM_TRIGGER_STOP:
        snd_card_mt85xx_pcm_playback_timer_stop(pcm);
        break;
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        break;
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        break;
    default:
        err = -EINVAL;
        break;
    }
    spin_unlock(&pcm->lock);
    return 0;
}

static void snd_card_mt85xx_pcm_playback_timer_function(unsigned long data)
{
    struct snd_mt85xx_pcm *pcm = (struct snd_mt85xx_pcm *)data;
    unsigned long flags;
    unsigned int pcm_old_rptr = pcm->pcm_rptr;
    unsigned int bytes_elapsed;

    int snd_pcm_device = pcm->substream->pcm->device;
    int snd_card_number = pcm->substream->pcm->card->number; 
    // Special workaround for USB camera pulg in USB 3.0, card0(mtk) will be chagned to card1
    if (snd_card_number==1)
    {
        //Printf("[ALSA] Cahnge snd_pcm_device from %d to %d for USB3.0 Camera plug-in\n", snd_pcm_device, snd_pcm_device-1);
        snd_pcm_device--;
    } 


    spin_lock_irqsave(&pcm->lock, flags);

    // setup next timer
    pcm->timer.expires = jiffies + pcm->pcm_period_size * HZ / (pcm->substream->runtime->rate * pcm->substream->runtime->channels * 2);        
    add_timer(&pcm->timer);

    // STEP 1: refresh read pointer
  #ifndef MTK_AUDIO_SUPPORT_MULTI_STREAMOUT 
    pcm->pcm_rptr = AUD_GetMixSndReadPtr(pcm->substream->number) - AUD_GetMixSndFIFOStart(pcm->substream->number);
  #else
    pcm->pcm_rptr = AUD_GetMixSndReadPtr(snd_pcm_device) - AUD_GetMixSndFIFOStart(snd_pcm_device);
  #endif

    // STEP 2: update write pointer
    snd_card_mt85xx_pcm_playback_update_write_pointer(pcm);

    // STEP 3: check period
    bytes_elapsed = pcm->pcm_rptr - pcm_old_rptr + pcm->pcm_buffer_size;
    bytes_elapsed %= pcm->pcm_buffer_size;
    pcm->bytes_elapsed += bytes_elapsed;

    // Printf("[ALSA] timer: pcm->bytes_elapsed = %d\n", pcm->bytes_elapsed);

    if (pcm->bytes_elapsed >= pcm->pcm_period_size)
    {
        pcm->bytes_elapsed %= pcm->pcm_period_size;
        spin_unlock_irqrestore(&pcm->lock, flags);

        snd_pcm_period_elapsed(pcm->substream);
    }
    else
    {
        spin_unlock_irqrestore(&pcm->lock, flags);
    }
}

static snd_pcm_uframes_t snd_card_mt85xx_pcm_playback_pointer(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct snd_mt85xx_pcm *pcm = runtime->private_data;

    // Printf("[ALSA] operator: pointer\n");

    return bytes_to_frames(runtime, pcm->pcm_rptr);
}


static struct snd_pcm_hardware snd_card_mt85xx_pcm_playback_hw =
{
    .info =            (SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
                        SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_MMAP_VALID),
    .formats =          USE_FORMATS,
    .rates =            USE_RATE,
    .rate_min =         USE_RATE_MIN,
    .rate_max =         USE_RATE_MAX,
    .channels_min =     USE_CHANNELS_MIN,
    .channels_max =     USE_CHANNELS_MAX,
    .buffer_bytes_max = MAX_BUFFER_SIZE,
    .period_bytes_min = MIN_PERIOD_SIZE,
    .period_bytes_max = MAX_PERIOD_SIZE,
    .periods_min =      USE_PERIODS_MIN,
    .periods_max =      USE_PERIODS_MAX,
    .fifo_size =        0,
};

#ifdef SUPPORT_NEW_TUNAL_MODE
static struct snd_pcm_hardware snd_card_mt85xx_pcm_playback_hw_AC3 =
{
    .info =            (SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
                        SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_MMAP_VALID),
    .formats =          USE_FORMATS,
    .rates =            USE_RATE,
    .rate_min =         USE_RATE_MIN,
    .rate_max =         USE_RATE_MAX,
    .channels_min =     USE_CHANNELS_MIN,
    .channels_max =     USE_CHANNELS_MAX,
    .buffer_bytes_max = MAX_BUFFER_SIZE_AC3,
    .period_bytes_min = MIN_PERIOD_SIZE_AC3,
    .period_bytes_max = MAX_PERIOD_SIZE_AC3,
    .periods_min =      USE_PERIODS_MIN_AC3,
    .periods_max =      USE_PERIODS_MAX_AC3,
    .fifo_size =        0,
};
#endif

static void snd_card_mt85xx_runtime_free(struct snd_pcm_runtime *runtime)
{
    kfree(runtime->private_data);
}

static int snd_card_mt85xx_pcm_playback_hw_params(struct snd_pcm_substream *substream,
                    struct snd_pcm_hw_params *hw_params)
{
    Printf("[ALSA] operator: hw_params\n");

    return 0;
}

static int snd_card_mt85xx_pcm_playback_buffer_bytes_rule(struct snd_pcm_hw_params *params,
                      struct snd_pcm_hw_rule *rule)
{
#if 1
    struct snd_interval *buffer_bytes;
    //Printf("[ALSA] buffer bytes rule\n");
    // fix buffer size to 128KB
    buffer_bytes = hw_param_interval(params, SNDRV_PCM_HW_PARAM_BUFFER_BYTES);
    // patch for android audio
    // allow alsa/oss client to shrink buffer size
    buffer_bytes->min = 512;
    buffer_bytes->max = MAX_BUFFER_SIZE;
#endif
    return 0;
}

#ifdef SUPPORT_NEW_TUNAL_MODE
static int snd_card_mt85xx_pcm_playback_buffer_bytes_rule_AC3(struct snd_pcm_hw_params *params,
                      struct snd_pcm_hw_rule *rule)
{
#if 1
    struct snd_interval *buffer_bytes;
    //Printf("[ALSA] buffer bytes rule\n");
    // fix buffer size to 128KB
    buffer_bytes = hw_param_interval(params, SNDRV_PCM_HW_PARAM_BUFFER_BYTES);
    // patch for android audio
    // allow alsa/oss client to shrink buffer size
    buffer_bytes->min = 512;
    buffer_bytes->max = MAX_BUFFER_SIZE_AC3;
#endif
    return 0;
}
#endif


static int snd_card_mt85xx_pcm_playback_hw_free(struct snd_pcm_substream *substream)
{
    Printf("[ALSA] operator: hw_free\n");

    return 0;
}

static struct snd_mt85xx_pcm *new_pcm_playback_stream(struct snd_pcm_substream *substream)
{
    struct snd_mt85xx_pcm *pcm;

    pcm = kzalloc(sizeof(*pcm), GFP_KERNEL);
    if (! pcm)
        return pcm;

    init_timer(&pcm->timer);
    pcm->timer.data = (unsigned long) pcm;
    pcm->timer.function = snd_card_mt85xx_pcm_playback_timer_function;

    spin_lock_init(&pcm->lock);

    pcm->substream = substream;
    pcm->instance = substream->number;

    return pcm;
}

static int snd_card_mt85xx_pcm_playback_open(struct snd_pcm_substream *substream)
{
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct snd_mt85xx_pcm *pcm;
    int err;

    int snd_pcm_device = substream->pcm->device;
    int snd_card_number = substream->pcm->card->number; 

    if ((pcm = new_pcm_playback_stream(substream)) == NULL)
        return -ENOMEM;

    // Special workaround for USB camera pulg in USB 3.0, card0(mtk) will be chagned to card1
    if (snd_card_number==1)
    {
        Printf("[ALSA] Open : change snd_pcm_device from %d to %d for USB3.0 Camera plug-in\n", snd_pcm_device, snd_pcm_device-1);
        snd_pcm_device--;
            if (snd_pcm_device<0)
            {
                snd_pcm_device = 0;
                Printf("[ALSA] Convert device number error for Card1!!!\n");
            }
    }        

    runtime->private_data = pcm;
    runtime->private_free = snd_card_mt85xx_runtime_free;
 #ifdef SUPPORT_NEW_TUNAL_MODE
   if ((snd_pcm_device == ALSA_TUNAL_AC3_DSPDEC_ID) | (snd_pcm_device == ALSA_NON_TUNAL_AC3_DSPDEC_ID))
       runtime->hw = snd_card_mt85xx_pcm_playback_hw_AC3;
   else 
    runtime->hw = snd_card_mt85xx_pcm_playback_hw;
 #else
       runtime->hw = snd_card_mt85xx_pcm_playback_hw;
 #endif

    Printf("[ALSA] operator: open, substream = 0x%X\n", (unsigned int) substream);
    Printf("[ALSA] substream->pcm->device = 0x%X\n", substream->pcm->device);
    Printf("[ALSA] snd_pcm_device = 0x%X\n", snd_pcm_device);
    Printf("[ALSA] substream->number = %d\n", substream->number);

#ifndef MTK_AUDIO_SUPPORT_MULTI_STREAMOUT
    if (snd_pcm_device & 1) {
        runtime->hw.info &= ~SNDRV_PCM_INFO_INTERLEAVED;
        runtime->hw.info |= SNDRV_PCM_INFO_NONINTERLEAVED;
    }
    if (snd_pcm_device & 2)
        runtime->hw.info &= ~(SNDRV_PCM_INFO_MMAP|SNDRV_PCM_INFO_MMAP_VALID);
#endif


#ifdef SUPPORT_NEW_TUNAL_MODE
      if ((snd_pcm_device == ALSA_TUNAL_AC3_DSPDEC_ID) | (snd_pcm_device == ALSA_NON_TUNAL_AC3_DSPDEC_ID))
      {
            err = snd_pcm_hw_rule_add(runtime, 0, SNDRV_PCM_HW_PARAM_BUFFER_BYTES,
                                            snd_card_mt85xx_pcm_playback_buffer_bytes_rule_AC3, NULL,
                                            SNDRV_PCM_HW_PARAM_CHANNELS, -1);
      }
	  else
	  {
            err = snd_pcm_hw_rule_add(runtime, 0, SNDRV_PCM_HW_PARAM_BUFFER_BYTES,
                                             snd_card_mt85xx_pcm_playback_buffer_bytes_rule, NULL,
                                             SNDRV_PCM_HW_PARAM_CHANNELS, -1);
	  }		
#else
    // add constraint rules
    err = snd_pcm_hw_rule_add(runtime, 0, SNDRV_PCM_HW_PARAM_BUFFER_BYTES,
                              snd_card_mt85xx_pcm_playback_buffer_bytes_rule, NULL,
                              SNDRV_PCM_HW_PARAM_CHANNELS, -1);
#endif

#ifndef MTK_AUDIO_SUPPORT_MULTI_STREAMOUT
    AUD_InitALSAPlayback_MixSnd(substream->number);
#else
    //CC_AUD_AV_TUNNEL: SW_MIXSND3 is for HW_AV_SYNC
    AUD_InitALSAPlayback_MixSnd(snd_pcm_device);
#endif
    return 0;
}

static int snd_card_mt85xx_pcm_playback_close(struct snd_pcm_substream *substream)
{
    //2012/12/7 added by daniel
    struct snd_pcm_runtime *runtime = substream->runtime;
    struct snd_mt85xx_pcm *pcm = runtime->private_data;
    
    int snd_pcm_device = substream->pcm->device;
    int snd_card_number = substream->pcm->card->number;         

    // Special workaround for USB camera pulg in USB 3.0, card0(mtk) will be chagned to card1
    if (snd_card_number==1)
    {
        Printf("[ALSA] Close : change snd_pcm_device from %d to %d for USB3.0 Camera plug-in\n", snd_pcm_device, snd_pcm_device-1);
        snd_pcm_device--;
            if (snd_pcm_device<0)
            {
                snd_pcm_device = 0;
                Printf("[ALSA] Convert device number error for Card1!!!\n");
            }
    }    
    
    
    if (pcm->timer_started)
    {
        del_timer_sync(&pcm->timer);
        pcm->timer_started = 0;
    } 

    Printf("[ALSA] operator: close, substream = 0x%X\n", (unsigned int) substream);

#ifndef MTK_AUDIO_SUPPORT_MULTI_STREAMOUT
    AUD_DeInitALSAPlayback_MixSnd(substream->number);
#else
    AUD_DeInitALSAPlayback_MixSnd(snd_pcm_device);
#endif
    return 0;
}

static struct snd_pcm_ops snd_card_mt85xx_playback_ops = {
    .open      = snd_card_mt85xx_pcm_playback_open,
    .close     = snd_card_mt85xx_pcm_playback_close,
    .ioctl     = snd_pcm_lib_ioctl,
    .hw_params = snd_card_mt85xx_pcm_playback_hw_params,
    .hw_free   = snd_card_mt85xx_pcm_playback_hw_free,
    .prepare   = snd_card_mt85xx_pcm_playback_prepare,
    .trigger   = snd_card_mt85xx_pcm_playback_trigger,
    .pointer   = snd_card_mt85xx_pcm_playback_pointer,
};

//Dan Zhou add on 20111125
static int snd_card_mt85xx_pcm_capture_open(struct snd_pcm_substream *substream)
{
    Printf("[ALSA]snd_card_mt85xx_pcm_capture_open----> operator: open, substream = 0x%X\n", (unsigned int) substream);
    
    return EPERM;
}

static int snd_card_mt85xx_pcm_capture_close(struct snd_pcm_substream *substream)
{
    Printf("[ALSA]snd_card_mt85xx_pcm_capture_close----> operator: close, substream = 0x%X\n", (unsigned int) substream);

    return 0;
}

static struct snd_pcm_ops snd_card_mt85xx_capture_ops = {
    .open      = snd_card_mt85xx_pcm_capture_open,
    .close     = snd_card_mt85xx_pcm_capture_close,
    .ioctl     = snd_pcm_lib_ioctl,
};

int __devinit snd_card_mt85xx_pcm(struct snd_mt85xx *mt85xx, int device, int substreams)
{
    struct snd_pcm *pcm;
    int err;

    if ((err = snd_pcm_new(mt85xx->card, "mt85xx PCM", device,
                   substreams, substreams, &pcm)) < 0)
        return err;

    mt85xx->pcm = pcm;

    snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_card_mt85xx_playback_ops);
    //Dan Zhou add on 20111125
    snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &snd_card_mt85xx_capture_ops);

    pcm->private_data = mt85xx;
    pcm->info_flags = 0;
    strcpy(pcm->name, "mtk PCM");

    return 0;
}


//===========================================================================//
#ifdef SUPPORT_COMPRESS_OFFLOAD

#define StreamID_FOR_COMPR    3 //Must be sync with aud_mixsnd.c : #define ALSA_DSPDEC_ID   3
#define USE_SPIN_LOCK
//#define USE_FIX_PARAMETERS  //for timer setting, need to take care ! it may cause system hang !

unsigned char *pucVirtBufAddr = NULL;
unsigned int pucPhysBufAddr = 0;
unsigned int app_pointer = 0;
unsigned int aproc_read_pointer = 0;
unsigned int aproc_read_pointer_old = 0;
unsigned int aproc_read_pointer_accu = 0;
unsigned int aproc_bytes_elapsed = 0;

//to do....
#if 0 //For AFIFO SIZE = 16K bytes
unsigned int compress_period_size = 4096;
unsigned int compress_buffer_size = 16384;
unsigned int compress_sampling_rate = 48000;
unsigned int compress_ch_out = 2;
#else //For AFIFO SIZE = 64K bytes
unsigned int compress_period_size = 4096;
unsigned int compress_buffer_size = 4096*16;
unsigned int compress_sampling_rate = 48000;
unsigned int compress_ch_out = 2;
#endif

static void snd_card_mt85xx_compress_playback_timer_start(struct snd_compr_stream *cstream)
{
    struct snd_compr_runtime *runtime = cstream->runtime;
    struct snd_mt85xx_pcm *pcm = runtime->private_data; 
#ifndef NEW_TIMER_FOR_COMPRESS_OFFLOAD 
    if (pcm->timer_started)
    {
        del_timer_sync(&pcm->timer);
        pcm->timer_started = 0;
    }
#ifdef USE_FIX_PARAMETERS 
    pcm->timer.expires = jiffies + compress_period_size * HZ / (compress_sampling_rate *compress_ch_out * 2);
#else
    //pcm->timer.expires = jiffies + (pcm->compr->runtime->fragment_size) * HZ / ((pcm->sampling_rate) *(pcm->channel_num) * 2); //Cause system hang for channel_num >=4
    pcm->timer.expires = jiffies + (pcm->compr->runtime->fragment_size) * HZ / ((pcm->sampling_rate) *(compress_ch_out) * 2);
#endif  
    add_timer(&pcm->timer); 
    pcm->timer_started = 1;
    pr_notice("[ALSA Compress] [%s], pcm->timer.expires=  %u\n", __func__, (unsigned int)pcm->timer.expires); 
 #else
    if (pcm->timer_for_compress_offload_started)
    {
        del_timer_sync(&pcm->timer_for_compress_offload);
        pcm->timer_for_compress_offload_started = 0;
    }
#ifdef USE_FIX_PARAMETERS 
    pcm->timer_for_compress_offload.expires = jiffies + compress_period_size * HZ / (compress_sampling_rate *compress_ch_out * 2);
#else
    //pcm->timer.expires = jiffies + (pcm->compr->runtime->fragment_size) * HZ / ((pcm->sampling_rate) *(pcm->channel_num) * 2); //Cause system hang for channel_num >=4
    pcm->timer_for_compress_offload.expires = jiffies + (pcm->compr->runtime->fragment_size) * HZ / ((pcm->sampling_rate) *(compress_ch_out) * 2);
#endif  
    add_timer(&pcm->timer_for_compress_offload);

    pcm->timer_for_compress_offload_started= 1;
    pr_notice("NEW[ALSA Compress] [%s], pcm->timer_for_compress_offload.expires=  %u\n", __func__, (unsigned int)pcm->timer_for_compress_offload.expires);
#endif
    
}

static void snd_card_mt85xx_compress_playback_timer_stop(struct snd_compr_stream *cstream)
{
    struct snd_compr_runtime *runtime = cstream->runtime;
    struct snd_mt85xx_pcm *pcm = runtime->private_data;

 #ifndef NEW_TIMER_FOR_COMPRESS_OFFLOAD 
    if (del_timer(&pcm->timer)==1)
    {
        pcm->timer_started = 0;
    }
 #else
    if (del_timer(&pcm->timer_for_compress_offload)==1)
    {
        pcm->timer_for_compress_offload_started= 0;
    }
 #endif
}


static void snd_card_mt85xx_compress_playback_timer_function(unsigned long data)
{
    struct snd_mt85xx_pcm *pcm = (struct snd_mt85xx_pcm *)data;
    unsigned long flags;
    unsigned int bytes_elapsed;

#if 0 //just for test
    unsigned int aproc_read_pointer_test;
    unsigned int aproc_start_address_test;
#endif

#ifdef NEW_LOCK_FOR_COMPRESS_OFFLOAD
    spin_lock_irqsave(&pcm->lock_for_compress_offload, flags);
#else
    spin_lock_irqsave(&pcm->lock, flags);
#endif

    // setup next timer
#ifdef USE_FIX_PARAMETERS  
#ifndef NEW_TIMER_FOR_COMPRESS_OFFLOAD 
    pcm->timer.expires = jiffies + (compress_period_size) * HZ / (compress_sampling_rate *compress_ch_out * 2);  
#else  
    pcm->timer_for_compress_offload.expires = jiffies + (compress_period_size) * HZ / (compress_sampling_rate *compress_ch_out * 2); 
#endif
#else  
#ifndef NEW_TIMER_FOR_COMPRESS_OFFLOAD      
    //pcm->timer.expires = jiffies + (pcm->compr->runtime->fragment_size) * HZ / (pcm->sampling_rate *pcm->channel_num* 2); ////Cause system hang for channel_num >=4
    pcm->timer.expires = jiffies + (pcm->compr->runtime->fragment_size) * HZ / (pcm->sampling_rate *compress_ch_out* 2);
#else
    //pcm->timer_for_compress_offload.expires = jiffies + (pcm->compr->runtime->fragment_size) * HZ / (pcm->sampling_rate *pcm->channel_num* 2); ////Cause system hang for channel_num >=4
    pcm->timer_for_compress_offload.expires = jiffies + (pcm->compr->runtime->fragment_size) * HZ / (pcm->sampling_rate *compress_ch_out* 2);
#endif
#endif 

    aproc_read_pointer = AUD_GetMixSndReadPtr(StreamID_FOR_COMPR) - AUD_GetMixSndFIFOStart(StreamID_FOR_COMPR);

#if 0 // just for test
    aproc_read_pointer_test = AUD_GetMixSndReadPtr(StreamID_FOR_COMPR) ;
    aproc_start_address_test = AUD_GetMixSndFIFOStart(StreamID_FOR_COMPR) ;
#endif   

#ifdef USE_FIX_PARAMETERS    
    bytes_elapsed = aproc_read_pointer - aproc_read_pointer_old + compress_buffer_size;    
    bytes_elapsed = (bytes_elapsed) % ((unsigned int)(compress_buffer_size));
#else
    bytes_elapsed = aproc_read_pointer - aproc_read_pointer_old + (pcm->compr->runtime->buffer_size);    
    bytes_elapsed = (bytes_elapsed) % ((unsigned int)(pcm->compr->runtime->buffer_size));
#endif    
    aproc_bytes_elapsed = aproc_bytes_elapsed + bytes_elapsed ;

    aproc_read_pointer_accu = aproc_read_pointer_accu + bytes_elapsed;    

#if 0 // just for test
    pr_notice("[ALSA Compress] [%s], aproc_read_pointer_test=  %x,  aproc_start_address_test= %x\n", __func__, aproc_read_pointer_test, aproc_start_address_test);
    pr_notice("[ALSA Compress] [%s], aproc_read_pointer=  %u,  aproc_read_pointer_old= %u, aproc_read_pointer_accu = %u\n", __func__, aproc_read_pointer, aproc_read_pointer_old, aproc_read_pointer_accu);
    pr_notice("[ALSA Compress] [%s], aproc_read_pointer=  %u,  aproc_read_pointer_old= %u, aproc_read_pointer_accu = %u, aproc_read_pointer_test=  %u ,  aproc_start_address_test= %u\n", __func__, aproc_read_pointer, aproc_read_pointer_old, aproc_read_pointer_accu,aproc_read_pointer_test,aproc_start_address_test);
#endif  

    aproc_read_pointer_old  = aproc_read_pointer;

#ifndef NEW_TIMER_FOR_COMPRESS_OFFLOAD      
    add_timer(&pcm->timer);
#else
    add_timer(&pcm->timer_for_compress_offload);
#endif

#ifdef USE_FIX_PARAMETERS    
    if (aproc_bytes_elapsed >= compress_period_size)
#else
    if (aproc_bytes_elapsed >= (pcm->compr->runtime->fragment_size))
#endif
    {
#ifdef USE_FIX_PARAMETERS    
        aproc_bytes_elapsed =  aproc_bytes_elapsed - compress_period_size;
#else
        aproc_bytes_elapsed =  aproc_bytes_elapsed - (pcm->compr->runtime->fragment_size);    
#endif

#ifdef NEW_LOCK_FOR_COMPRESS_OFFLOAD  
        spin_unlock_irqrestore(&pcm->lock_for_compress_offload, flags);
#else
        spin_unlock_irqrestore(&pcm->lock, flags);
#endif

        snd_compr_fragment_elapsed(pcm->compr);
    }
    else
    {
#ifdef NEW_LOCK_FOR_COMPRESS_OFFLOAD      
        spin_unlock_irqrestore(&pcm->lock_for_compress_offload, flags);
#else
        spin_unlock_irqrestore(&pcm->lock, flags);
#endif
    }


}

static struct snd_mt85xx_pcm *new_compress_playback_stream(struct snd_compr_stream *cstream)
{
    struct snd_mt85xx_pcm *pcm;

    pcm = kzalloc(sizeof(*pcm), GFP_KERNEL);
    if (! pcm)
        return pcm;

 #ifndef NEW_TIMER_FOR_COMPRESS_OFFLOAD     
    init_timer(&pcm->timer);
    pcm->timer.data = (unsigned long) pcm;
    pcm->timer.function = snd_card_mt85xx_compress_playback_timer_function;
 #else
    init_timer(&pcm->timer_for_compress_offload);
    pcm->timer_for_compress_offload.data = (unsigned long) pcm;
    pcm->timer_for_compress_offload.function = snd_card_mt85xx_compress_playback_timer_function;
 #endif

 #ifdef NEW_LOCK_FOR_COMPRESS_OFFLOAD        
    spin_lock_init(&pcm->lock_for_compress_offload);
 #else 
    spin_lock_init(&pcm->lock);
 #endif

    pcm->compr = cstream;

    return pcm;
}


static int mtk_compress_pcm_start(struct snd_compr_stream *cstream)
{
    pr_notice("[ALSA Compress]  [%s]\n", __func__);
	
    AUD_PlayMixSndRingFifo(StreamID_FOR_COMPR, 48000, 1, 16, cstream->runtime->buffer_size );

    return 0;
}

static int mtk_compress_pcm_stop(struct snd_compr_stream *cstream)
{
    pr_notice("[ALSA Compress] [%s]\n", __func__);

    app_pointer = 0;
    aproc_read_pointer = 0 ; 
    aproc_read_pointer_old = 0;
    aproc_read_pointer_accu= 0;	
	
    AUD_DeInitALSAPlayback_MixSnd(StreamID_FOR_COMPR);
	
    return 0;
}

static int mtk_compress_pcm_pause(struct snd_compr_stream *cstream)
{
    pr_notice("[ALSA Compress]  [%s]\n", __func__);
    AUD_SetDSPDecoderPause();
    return 0;
}

static int mtk_compress_pcm_resume(struct snd_compr_stream *cstream)
{
    pr_notice("[ALSA Compress]  [%s]\n", __func__);
    AUD_SetDSPDecoderResume();
    return 0;
}

static int mtk_compress_compr_open(struct snd_compr_stream *cstream)
{
    int ret_val = 0;
#ifdef USE_SPIN_LOCK
    struct snd_mt85xx_pcm *pcm;
#endif 

    pr_notice("[ALSA Compress] %s\n", __func__); 
    if ((pcm = new_compress_playback_stream(cstream)) == NULL)
    {
        pr_notice("[ALSA Compress] %s   !!!!! Cant't allocate pcm  !!!!! \n", __func__);
        return 1; 
    } 

    cstream->runtime->private_data = pcm;
    AUD_InitALSAPlayback_MixSnd(StreamID_FOR_COMPR);
    return ret_val;
}

static int mtk_compress_compr_free(struct snd_compr_stream *cstream)
{
    struct snd_compr_runtime *runtime = cstream->runtime;
    struct snd_mt85xx_pcm *pcm = runtime->private_data; 

    pr_notice("[ALSA Compress] %s\n", __func__); 

#ifndef NEW_TIMER_FOR_COMPRESS_OFFLOAD 
    if (pcm->timer_started)
    {
        del_timer_sync(&pcm->timer);
        pcm->timer_started = 0;
    }
#else
    if (pcm->timer_for_compress_offload_started)
    {
        del_timer_sync(&pcm->timer_for_compress_offload);
        pcm->timer_for_compress_offload_started= 0;
    }
#endif

    AUD_DeInitALSAPlayback_MixSnd(StreamID_FOR_COMPR);
    mtk_compress_pcm_stop(cstream);

      //Need to Free Allocated ALSA Kernel Buffer
    if ((pucVirtBufAddr != 0) && (pucPhysBufAddr != 0)) {
        dma_free_coherent(0, runtime->buffer_size, pucVirtBufAddr, pucPhysBufAddr);
        pucVirtBufAddr = NULL;
        pucPhysBufAddr = 0;
        //pr_notice("[ALSA Compress]%s free %d memory\n", __func__, runtime->buffer_size);
    }
     
    return 0;
}

static int mtk_compress_compr_set_params(struct snd_compr_stream *cstream,
                    struct snd_compr_params *params)
{ 
    struct snd_compr_runtime *runtime = cstream->runtime; 
    struct snd_mt85xx_pcm *pcm = runtime->private_data;
    int   samplerate; 

    pr_notice("[ALSA Compress] %s\n", __func__); 
    //Need to allocate/Setting ALSA Kernel Buffer
    pr_notice("[ALSA Compress] [%s] buffer_size: %llu\n", __func__, cstream->runtime->buffer_size);
    pr_notice("[ALSA Compress] [%s] fragment_size: %u\n", __func__, cstream->runtime->fragment_size);
    pr_notice("[ALSA Compress] [%s] fragments: %u\n", __func__, cstream->runtime->fragments);
    pr_notice("[ALSA Compress] [%s] allocate %llu bytes\n", __func__, (cstream->runtime->buffer_size));

    /* allocate memory */
    pucVirtBufAddr  = (unsigned char *)AUD_GetMixSndFIFOStart(StreamID_FOR_COMPR);
    pr_notice("[ALSA Compress] allocate memory: pucVirtBufAddr 0x%x\n",  (unsigned int) pucVirtBufAddr);


#if  0 //just for test
    pr_notice("[ALSA Compress] AudioRingFifoStart: 0x%x\n", AUD_GetMixSndFIFOStart(StreamID_FOR_COMPR));
    pr_notice("[ALSA Compress] AudioRingFifoEndt:  0x%x\n", AUD_GetMixSndFIFOEnd(StreamID_FOR_COMPR));     
#endif

    if (cstream->runtime->buffer_size > (AUD_GetMixSndFIFOEnd(StreamID_FOR_COMPR) - AUD_GetMixSndFIFOStart(StreamID_FOR_COMPR)))
    {
        // buffer size must match
        return -EINVAL;
    }
    //spin_lock_irqsave(&pcm->lock, flags);

    switch (params->codec.id) {
    case SND_AUDIOCODEC_PCM:
        AUD_SetDSPDecoderType(AUD_FMT_PCM);
        pr_notice("[ALSA Compress] SND_AUDIOCODEC_PCM\n");
        break;
    case SND_AUDIOCODEC_MP3:
        AUD_SetDSPDecoderType(AUD_FMT_MP3);
        pr_notice("[ALSA Compress] SND_AUDIOCODEC_MP3\n");
        break;
    case SND_AUDIOCODEC_AMR:
        AUD_SetDSPDecoderType(AUD_FMT_AMR);
        pr_notice("[ALSA Compress] SND_AUDIOCODEC_AMR\n");
        break;
    case SND_AUDIOCODEC_AMRWB:
        AUD_SetDSPDecoderType(AUD_FMT_AMR);
        pr_notice("[ALSA Compress] SND_AUDIOCODEC_AMRWB\n");
        break;
    case SND_AUDIOCODEC_AMRWBPLUS:
        AUD_SetDSPDecoderType(AUD_FMT_AMR);                 
        pr_notice("[ALSA Compress] SND_AUDIOCODEC_AMRWBPLUS\n");
        break;
    case SND_AUDIOCODEC_AAC:
        AUD_SetDSPDecoderType(AUD_FMT_AAC);                 
        pr_notice("[ALSA Compress] SND_AUDIOCODEC_AAC\n");
        break;
    case SND_AUDIOCODEC_WMA:
        AUD_SetDSPDecoderType(AUD_FMT_WMA);                 
        pr_notice("[ALSA Compress] SND_AUDIOCODEC_WMA\n");
        break;
    case SND_AUDIOCODEC_REAL:
        AUD_SetDSPDecoderType(AUD_FMT_RA);                  
        pr_notice("[ALSA Compress] SND_AUDIOCODEC_REAL\n");
        break;
    case SND_AUDIOCODEC_VORBIS:
        AUD_SetDSPDecoderType(AUD_FMT_VORBISDEC);                   
        pr_notice("[ALSA Compress] SND_AUDIOCODEC_VORBIS\n");
        break;
    case SND_AUDIOCODEC_FLAC:
        AUD_SetDSPDecoderType(AUD_FMT_FLAC);                    
        pr_notice("[ALSA Compress] SND_AUDIOCODEC_FLAC\n");
        break;
    case SND_AUDIOCODEC_IEC61937:
        AUD_SetDSPDecoderType(AUD_FMT_AC3);// ??? DTS/AAC/AC3   
                                                                    //QCom的kernel的compress_params.h裡面有定義ac3                  
        pr_notice("[ALSA Compress] SND_AUDIOCODEC_IEC61937\n");
        break;
    default:
        pr_err("[ALSA Compress] !!!! codec not supported, id =%d\n", params->codec.id);
        return -EINVAL;
    }

    switch (params->codec.sample_rate) {
    case SNDRV_PCM_RATE_5512: {
        AUD_SetDSPSampleRate(FS_5K);                                   
        samplerate = 5512;      
        pcm->sampling_rate = 5512;
        pr_notice("[ALSA Compress] SNDRV_PCM_RATE_5512\n");
        break;
    }
    case SNDRV_PCM_RATE_8000: {
        AUD_SetDSPSampleRate(FS_8K);                                                   
        samplerate = 8000;   
        pcm->sampling_rate = 8000;          
        pr_notice("[ALSA Compress] SNDRV_PCM_RATE_8000\n");
        break;
    }   
    case SNDRV_PCM_RATE_11025: {
        AUD_SetDSPSampleRate(FS_11K);                                                                  
        samplerate = 11025;     
        pcm->sampling_rate = 11025;                     
        pr_notice("[ALSA Compress] SNDRV_PCM_RATE_11025\n");
        break;
    }
    case SNDRV_PCM_RATE_16000: {
        AUD_SetDSPSampleRate(FS_16K);                                                                                  
        samplerate = 16000;     
        pcm->sampling_rate = 16000;                                 
        pr_notice("[ALSA Compress] SNDRV_PCM_RATE_16000\n");
        break;
    }
    case SNDRV_PCM_RATE_22050: {
        AUD_SetDSPSampleRate(FS_22K);                                                                                                  
        samplerate = 22050;     
        pcm->sampling_rate = 22050;                                 
        pr_notice("[ALSA Compress] SNDRV_PCM_RATE_22050\n");
        break;
    }
    case SNDRV_PCM_RATE_32000: {
        AUD_SetDSPSampleRate(FS_32K);                                                                                                                  
        samplerate = 32000;     
        pcm->sampling_rate = 32000;                                             
        pr_notice("[ALSA Compress] SNDRV_PCM_RATE_32000\n");
        break;
    }
    case SNDRV_PCM_RATE_44100: {
        AUD_SetDSPSampleRate(FS_44K);                                                                                                                  
        samplerate = 44100;   
        pcm->sampling_rate = 44100;                                             
        pr_notice("[ALSA Compress] SNDRV_PCM_RATE_44100\n");
        break;
    }
    case SNDRV_PCM_RATE_48000: {
        AUD_SetDSPSampleRate(FS_48K);                                                                                                                                  
        samplerate = 48000;     
        pcm->sampling_rate = 48000;                                             
        pr_notice("[ALSA Compress] SNDRV_PCM_RATE_48000\n");
        break;
    }
    case SNDRV_PCM_RATE_64000: {
        AUD_SetDSPSampleRate(FS_64K);                                                                                                                                                  
        samplerate = 64000;     
        pcm->sampling_rate = 64000;                                             
        pr_notice("[ALSA Compress] SNDRV_PCM_RATE_64000\n");
        break;
    }
    case SNDRV_PCM_RATE_88200: {
        AUD_SetDSPSampleRate(FS_88K);                                                                                                                                                  
        samplerate = 88200;     
        pcm->sampling_rate = 88200;                                             
        pr_notice("[ALSA Compress] SNDRV_PCM_RATE_88200\n");
        break;
    }
    case SNDRV_PCM_RATE_96000: {
        AUD_SetDSPSampleRate(FS_96K);                                                                                                                                                                  
        samplerate = 96000;     
        pcm->sampling_rate = 96000;                                             
        pr_notice("[ALSA Compress] SNDRV_PCM_RATE_96000\n");
        break;
    }
    case SNDRV_PCM_RATE_176400: {
        AUD_SetDSPSampleRate(FS_176K);                                                                                                                                                                                 
        samplerate = 176400;  
        pcm->sampling_rate = 176400;                                                
        pr_notice("[ALSA Compress] SNDRV_PCM_RATE_176400\n");
        break;
    }
    case SNDRV_PCM_RATE_192000: {
        AUD_SetDSPSampleRate(FS_192K);                                                                                                                                                                                                 
        samplerate = 192000;  
        pcm->sampling_rate = 192000;                                                
        pr_notice("[ALSA Compress] SNDRV_PCM_RATE_192000\n");
        break;
    }             
    default:
        pr_err("[ALSA Compress] !!!!codec not supported, sample_rate =%d\n", params->codec.sample_rate);
        return -EINVAL;
    }
       // spin_unlock_irqrestore(&pcm->lock, flags);

    AUD_SetDSPChannelNum(params->codec.ch_in);                                              
    pcm->channel_num= params->codec.ch_in;
    pr_notice("[ALSA Compress] Channel_Number = %d\n", params->codec.ch_in);


#if 0
    AUD_PlayMixSndRingFifo(StreamID_FOR_COMPR, samplerate, 1, 16, cstream->runtime->buffer_size );
#else
  #if 0
    AUD_PlayMixSndRingFifo(StreamID_FOR_COMPR, 48000, 1, 16, cstream->runtime->buffer_size );
  #endif
#endif

    return 0;
}

static int mtk_compress_compr_get_params(struct snd_compr_stream *cstream,
                    struct snd_codec *params)
{
    pr_notice("[ALSA Compress] %s\n", __func__);

    return 0;
}

static int mtk_compress_compr_trigger(struct snd_compr_stream *cstream, int cmd)
{ 
    struct snd_compr_runtime *runtime = cstream->runtime;
    struct snd_mt85xx_pcm *pcm = runtime->private_data;
    unsigned long flags; 

    pr_notice("[ALSA Compress] NEW %s: cmd = %d\n", __func__, cmd);

 #ifdef NEW_LOCK_FOR_COMPRESS_OFFLOAD           
      //spin_lock(&pcm->lock_for_compress_offload);
      //spin_lock_irqsave(&pcm->lock_for_compress_offload, flags);
 #else
      spin_lock(&pcm->lock);
 #endif

    switch(cmd)
    {
    case SNDRV_PCM_TRIGGER_START:
        pr_notice("%s: SNDRV_PCM_TRIGGER_START\n", __func__);
        mtk_compress_pcm_start(cstream);
        snd_card_mt85xx_compress_playback_timer_start(cstream); 
        break;
    case SNDRV_PCM_TRIGGER_STOP:
        pr_notice("%s: SNDRV_PCM_TRIGGER_STOP\n", __func__);
        snd_card_mt85xx_compress_playback_timer_stop(cstream);           		
        mtk_compress_pcm_stop(cstream);
        break;
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	spin_lock_irqsave(&pcm->lock_for_compress_offload, flags);	
        pr_notice("%s: SNDRV_PCM_TRIGGER_PAUSE_PUSH\n", __func__);  
        mtk_compress_pcm_pause(cstream);
        snd_card_mt85xx_compress_playback_timer_stop(cstream);    
	spin_unlock_irqrestore(&pcm->lock_for_compress_offload, flags);	
        break;  
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
	spin_lock_irqsave(&pcm->lock_for_compress_offload, flags);	
        pr_notice("%s: SNDRV_PCM_TRIGGER_PAUSE_RELEASE\n", __func__);
        snd_card_mt85xx_compress_playback_timer_start(cstream);          
        mtk_compress_pcm_resume(cstream);       
	spin_unlock_irqrestore(&pcm->lock_for_compress_offload, flags);	
        break;              
      case SND_COMPR_TRIGGER_DRAIN:
	 spin_lock_irqsave(&pcm->lock_for_compress_offload, flags);	
        pr_notice("%s: SND_COMPR_TRIGGER_DRAIN\n", __func__);
       spin_unlock_irqrestore(&pcm->lock_for_compress_offload, flags);   
        break;
    default:
        break;
    }

 #ifdef NEW_LOCK_FOR_COMPRESS_OFFLOAD           
     // spin_unlock(&pcm->lock_for_compress_offload);
      //spin_unlock_irqrestore(&pcm->lock_for_compress_offload, flags);
 #else
      spin_unlock(&pcm->lock);
 #endif

    return 0;
}

static int mtk_compress_compr_pointer(struct snd_compr_stream *cstream,
                    struct snd_compr_tstamp *tstamp)
{
    unsigned int bytes_elapsed = 0;
    unsigned long flags; 
    
#ifdef USE_SPIN_LOCK       
    struct snd_compr_runtime *runtime = cstream->runtime;
    struct snd_mt85xx_pcm *pcm = runtime->private_data;
#endif

#if 0//just for test
    unsigned int aproc_read_pointer_test;
    unsigned int aproc_start_address_test;
#endif
      
#ifdef ENABLE_ALSA_LOG  
    pr_notice("[ALSA Compress] [%s], direction: %d\n", __func__, cstream->direction);
#endif

//Need to check How many bytes does DSP consumed 
#ifdef USE_SPIN_LOCK 
#ifdef NEW_LOCK_FOR_COMPRESS_OFFLOAD            
    //spin_lock(&pcm->lock_for_compress_offload);
    spin_lock_irqsave(&pcm->lock_for_compress_offload, flags);
#else
    spin_lock(&pcm->lock);
#endif
#endif

    aproc_read_pointer = AUD_GetMixSndReadPtr(StreamID_FOR_COMPR) - AUD_GetMixSndFIFOStart(StreamID_FOR_COMPR);

#if 0 // just for test
    aproc_read_pointer_test = AUD_GetMixSndReadPtr(StreamID_FOR_COMPR) ;
    aproc_start_address_test = AUD_GetMixSndFIFOStart(StreamID_FOR_COMPR) ;
#endif    

       
#ifdef USE_FIX_PARAMETERS 
    bytes_elapsed = aproc_read_pointer - aproc_read_pointer_old + compress_buffer_size;    
    bytes_elapsed = (bytes_elapsed) % ((unsigned int)(compress_buffer_size));
#else
    bytes_elapsed = aproc_read_pointer - aproc_read_pointer_old + cstream->runtime->buffer_size;    
    bytes_elapsed = (bytes_elapsed) % ((unsigned int)(cstream->runtime->buffer_size));
#endif  

    aproc_bytes_elapsed = aproc_bytes_elapsed + bytes_elapsed ;
    aproc_read_pointer_accu = aproc_read_pointer_accu + bytes_elapsed;    

//#ifdef ENABLE_ALSA_LOG
       //pr_notice("[ALSA Compress] [%s], aproc_read_pointer_test=  %x,  aproc_start_address_test= %x\n", __func__, aproc_read_pointer_test, aproc_start_address_test);
       //pr_notice("[ALSA Compress] [%s], aproc_read_pointer=  %u,  aproc_read_pointer_old= %u, aproc_read_pointer_accu = %u\n", __func__, aproc_read_pointer, aproc_read_pointer_old, aproc_read_pointer_accu);
       //pr_notice("[ALSA Compress] [%s], aproc_read_pointer=  %u,  aproc_read_pointer_old= %u, aproc_read_pointer_accu = %u, aproc_read_pointer_test=  %u ,  aproc_start_address_test= %u\n", __func__, aproc_read_pointer, aproc_read_pointer_old, aproc_read_pointer_accu,aproc_read_pointer_test,aproc_start_address_test);    
//#endif
          
    aproc_read_pointer_old  = aproc_read_pointer;

    tstamp->copied_total = aproc_read_pointer_accu;

#ifdef USE_SPIN_LOCK   
#ifdef NEW_LOCK_FOR_COMPRESS_OFFLOAD               
    //spin_unlock(&pcm->lock_for_compress_offload);
    spin_unlock_irqrestore(&pcm->lock_for_compress_offload, flags);
#else
    spin_unlock(&pcm->lock);
#endif
#endif

  return 0; 
}

static int mtk_compress_compr_ack(struct snd_compr_stream *cstream,
                    size_t bytes)
{
    unsigned char *src = NULL, *dstn = NULL;
    unsigned long flags;
	unsigned int copy;
    unsigned int app_pointer_temp = 0;
    unsigned int read_pointer = 0;
#ifdef USE_SPIN_LOCK       
    struct snd_compr_runtime *runtime = cstream->runtime;
    struct snd_mt85xx_pcm *pcm = runtime->private_data;
#endif     

#ifdef ENABLE_ALSA_LOG
    pr_notice("[ALSA Compress] [%s]Info DSP that data (%u bytes) has been written\n", __func__, bytes);
#endif  

    //Copy From ALSA Kernel Buffer to another Kernel Buffer 
#ifdef USE_SPIN_LOCK  
    //spin_lock(&pcm->lock);
#ifdef NEW_LOCK_FOR_COMPRESS_OFFLOAD                
    spin_lock_irqsave(&pcm->lock_for_compress_offload, flags);                 
#else
    spin_lock_irqsave(&pcm->lock, flags);                 
#endif
#endif
       
    app_pointer_temp = (app_pointer) % ((unsigned int)(cstream->runtime->buffer_size));

#ifdef ENABLE_ALSA_LOG   
    pr_notice("[ALSA Compress] [%s]app_pointer = %u , cstream->runtime->buffer_size = %llu, app_pointer_temp = %u \n", __func__, app_pointer, cstream->runtime->buffer_size, app_pointer_temp);
#endif
       
    dstn = pucVirtBufAddr+ app_pointer_temp ;
    src = cstream->runtime->buffer+ app_pointer_temp ;

#ifdef ENABLE_ALSA_LOG    
    pr_notice("[ALSA Compress][%s] dstn: 0x%x, src: 0x%x, app_pointer: 0x%x\n", __func__, (unsigned int)dstn, (unsigned int)src, app_pointer);
#endif

#if 1
    // copy buffer
          if (bytes < (cstream->runtime->buffer_size - app_pointer_temp))
	    {
		 memcpy((void *)dstn, (void *)src, bytes);
	    } 
          else 
	    {
		 copy = cstream->runtime->buffer_size - app_pointer_temp;
		 memcpy((void *)dstn, (void *)src, copy);
		 memcpy(pucVirtBufAddr, cstream->runtime->buffer, bytes - copy);
	    }
#else   
    // copy buffer
    x_memcpy((void *)(VIRTUAL((unsigned int)dstn)), (void *)(VIRTUAL((unsigned int)src)), bytes);
#endif

    app_pointer_temp = app_pointer_temp +  bytes;       
    app_pointer_temp = (app_pointer_temp) % ((unsigned int)(cstream->runtime->buffer_size)); 

    read_pointer = AUD_GetMixSndReadPtr(StreamID_FOR_COMPR) - AUD_GetMixSndFIFOStart(StreamID_FOR_COMPR);

#ifdef ENABLE_ALSA_LOG
    pr_notice("[ALSA Compress] [%s], read_pointer=  %u\n", __func__,read_pointer);
#endif

    if(bytes == cstream->runtime->buffer_size) 
    {
#ifdef ENABLE_ALSA_LOG     
        pr_notice("[ALSA Compress] Enter Buffer Empty case \n");
#endif      
        AUD_SetMixSndWritePtr(StreamID_FOR_COMPR, AUD_GetMixSndFIFOStart(StreamID_FOR_COMPR) + ((app_pointer_temp + (unsigned int)cstream->runtime->buffer_size - 1) % ((unsigned int)cstream->runtime->buffer_size)));

    }      
    else
    {
#ifdef ENABLE_ALSA_LOG        
        pr_notice("[ALSA Compress] Enter NON Buffer Empty case \n");
        pr_notice("[ALSA Compress] [%s] Update AUD_SetMixSndWritePtr: app_pointer_temp = %u \n", __func__, app_pointer_temp);
#endif

        if (app_pointer_temp == 0)
            AUD_SetMixSndWritePtr(StreamID_FOR_COMPR, AUD_GetMixSndFIFOStart(StreamID_FOR_COMPR) + ((app_pointer_temp + (unsigned int)cstream->runtime->buffer_size - 1) % ((unsigned int)cstream->runtime->buffer_size)));
        else
            AUD_SetMixSndWritePtr(StreamID_FOR_COMPR, AUD_GetMixSndFIFOStart(StreamID_FOR_COMPR) + app_pointer_temp-1);
    }
     

    app_pointer += bytes;

#ifdef USE_SPIN_LOCK          
    //spin_unlock(&pcm->lock);
#ifdef NEW_LOCK_FOR_COMPRESS_OFFLOAD                      
    spin_unlock_irqrestore(&pcm->lock_for_compress_offload, flags);      
#else
    spin_unlock_irqrestore(&pcm->lock, flags);      
#endif
#endif 

    return 0;
}
 

static int mtk_compress_compr_get_caps(struct snd_compr_stream *cstream,
                    struct snd_compr_caps *caps)
{
    pr_notice("[ALSA Compress] %s\n", __func__);

    return 0;
}

static int mtk_compress_compr_get_codec_caps(struct snd_compr_stream *cstream,
                    struct snd_compr_codec_caps *codec)
{
    pr_notice("[ALSA Compress] %s\n", __func__);

    return 0;
}

static struct snd_compr_ops mtk_compress_compr_ops = {
    .open = mtk_compress_compr_open,
    .free = mtk_compress_compr_free,
    .set_params = mtk_compress_compr_set_params,
    .get_params = mtk_compress_compr_get_params,
    .trigger = mtk_compress_compr_trigger,
    .pointer = mtk_compress_compr_pointer,
    .ack = mtk_compress_compr_ack,
//  .copy = mtk_compress_compr_copy,
    .get_caps = mtk_compress_compr_get_caps,
    .get_codec_caps = mtk_compress_compr_get_codec_caps,
};

int snd_card_mt85xx_compress(struct snd_mt85xx *mt85xx, int device, int substreams)
{
    struct snd_compr *compr;    
    int ret = 0;    

#ifdef  ENABLE_DETAIL_LOG
    printk("[ALSA] snd_card_mt85xx_compress()  ###### 1  ### STARTT \n");
#endif

    compr = kzalloc(sizeof(*compr), GFP_KERNEL);
    if (compr == NULL) {
        snd_printk(KERN_ERR "Cannot allocate compr\n");
        return -ENOMEM;
    }

#ifdef  ENABLE_DETAIL_LOG
    printk("[ALSA] snd_card_mt85xx_compress()  ###### 2 \n");
#endif
   #if 0
    compr->ops = &mtk_compress_compr_ops;
   #else
    compr->ops = kzalloc(sizeof(mtk_compress_compr_ops),GFP_KERNEL);
                  
    if (compr->ops == NULL) {
        snd_printk(KERN_ERR "Cannot allocate comp->opsr\n");
        return -ENOMEM;
    }
    memcpy(compr->ops, &mtk_compress_compr_ops, sizeof(mtk_compress_compr_ops));
   #endif
   
#ifdef  ENABLE_DETAIL_LOG
    printk("[ALSA] snd_card_mt85xx_compress()  ###### 3 \n");
#endif

    mutex_init(&compr->lock);
    ret = snd_compress_new(mt85xx->card, device, SND_COMPRESS_PLAYBACK, compr);
     
    if (ret < 0) {
        pr_err("compress asoc: can't create compress\n");
#ifdef  ENABLE_DETAIL_LOG
        printk("[ALSA] snd_card_mt85xx_compress()  ###### 4  ### ERROR !! \n");
#endif
    
        goto compr_err;
    }
    
#ifdef  ENABLE_DETAIL_LOG
    printk("[ALSA] snd_card_mt85xx_compress()  ###### 5 ### END \n");
#endif

    mt85xx->compr = compr;
    compr->private_data = mt85xx;
    return ret;

compr_err:
   #if 1
    kfree(compr->ops);
   #endif
    
    kfree(compr);
    return ret;

}
#endif


