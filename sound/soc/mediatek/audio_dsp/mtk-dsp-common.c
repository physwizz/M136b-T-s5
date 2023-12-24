// SPDX-License-Identifier: GPL-2.0
//
// Copyright (C) 2018 MediaTek Inc.
// Author: Chipeng Chang <chipeng.chang@mediatek.com>

#include <sound/soc.h>
#include <linux/device.h>
#include <linux/notifier.h>

/* ipi message related*/
#include <audio_ipi_dma.h>
#include <audio_ipi_platform.h>
#include <audio_messenger_ipi.h>
#include <audio_task_manager.h>
#include <audio_task.h>
#include <mtk-base-afe.h>

#include <mtk-dsp-mem-control.h>
#include <mtk-base-dsp.h>
#include <mtk-dsp-common.h>

#ifdef CONFIG_MTK_AUDIODSP_SUPPORT
#include <adsp_helper.h>
#else
#include <scp_helper.h>
#endif

//#define DEBUG_VERBOSE

/* don't use this directly if not necessary */
static struct mtk_base_dsp *local_base_dsp;
static struct mtk_base_afe *local_dsp_afe;

int audio_set_dsp_afe(struct mtk_base_afe *afe)
{
	local_dsp_afe = afe;
	return 0;
}

struct mtk_base_afe *get_afe_base(void)
{
	if (local_dsp_afe == NULL)
		pr_info("%s local_base_dsp == NULL", __func__);

	return local_dsp_afe;
}

int set_dsp_base(struct mtk_base_dsp *pdsp)
{
	if (pdsp == NULL)
		pr_info("%s pdsp == NULL", __func__);

	local_base_dsp = pdsp;
	return 0;
}

void *get_dsp_base(void)
{
	if (local_base_dsp == NULL)
		pr_warn("%s local_base_dsp == NULL", __func__);
	return local_base_dsp;
}

static void *ipi_recv_private;
void *get_ipi_recv_private(void)
{
	return ipi_recv_private;
}

void set_ipi_recv_private(void *priv)
{
	pr_debug("%s\n", __func__);

	if (priv != NULL)
		ipi_recv_private = priv;
	else
		pr_debug("%s ipi_recv_private has been set\n", __func__);
}

int copy_ipi_payload(void *dst, void *src, unsigned int size)
{
	if (size > MAX_PAYLOAD_SIZE)
		return -1;

	memcpy(dst, src, size);
	return 0;
}

/*
 * common function for IPI message
 */
int mtk_scp_ipi_send(int task_scene, int data_type, int ack_type,
		     uint16_t msg_id, uint32_t param1, uint32_t param2,
		     char *payload)
{
	struct ipi_msg_t ipi_msg;
	int send_result = 0;

	memset((void *)&ipi_msg, 0, sizeof(struct ipi_msg_t));

	if (!is_audio_task_dsp_ready(task_scene)) {
		pr_info("%s(), is_adsp_ready send false\n", __func__);
		send_result = -1;
		return send_result;
	}

	if (get_task_attr(get_dspdaiid_by_dspscene(task_scene),
			  ADSP_TASK_ATTR_DEFAULT) == 0) {
		pr_info("%s() task_scene[%d] not enable\n",
			__func__, task_scene);
		send_result = -1;
		return send_result;
	}

	send_result = audio_send_ipi_msg(
		&ipi_msg, task_scene,
		AUDIO_IPI_LAYER_TO_DSP, data_type,
		ack_type, msg_id, param1, param2,
		(char *)payload);

	if (send_result)
		pr_info("%s(),scp_ipi send fail\n",
			__func__);

	return send_result;
}

/* dsp scene ==> od mapping */
int get_dspscene_by_dspdaiid(int id)
{
	switch (id) {
	case AUDIO_TASK_VOIP_ID:
		return TASK_SCENE_VOIP;
	case AUDIO_TASK_PRIMARY_ID:
		return TASK_SCENE_PRIMARY;
	case AUDIO_TASK_OFFLOAD_ID:
		return TASK_SCENE_PLAYBACK_MP3;
	case AUDIO_TASK_DEEPBUFFER_ID:
		return TASK_SCENE_DEEPBUFFER;
	case AUDIO_TASK_PLAYBACK_ID:
		return TASK_SCENE_AUDPLAYBACK;
	case AUDIO_TASK_CAPTURE_UL1_ID:
		return TASK_SCENE_CAPTURE_UL1;
	case AUDIO_TASK_A2DP_ID:
		return TASK_SCENE_A2DP;
	case AUDIO_TASK_DATAPROVIDER_ID:
		return TASK_SCENE_DATAPROVIDER;
	case AUDIO_TASK_CALL_FINAL_ID:
		return TASK_SCENE_CALL_FINAL;
	case AUDIO_TASK_FAST_ID:
		return TASK_SCENE_FAST;
	case AUDIO_TASK_MUSIC_ID:
		return TASK_SCENE_MUSIC;
	case AUDIO_TASK_KTV_ID:
		return TASK_SCENE_KTV;
	case AUDIO_TASK_CAPTURE_RAW_ID:
		return TASK_SCENE_CAPTURE_RAW;
	case AUDIO_TASK_FM_ADSP_ID:
		return TASK_SCENE_FM_ADSP;
	case AUDIO_TASK_UL_PROCESS_ID:
		return TASK_SCENE_UL_PROCESS;
	case AUDIO_TASK_ECHO_REF_ID:
		return TASK_SCENE_ECHO_REF_UL;
	case AUDIO_TASK_ECHO_REF_DL_ID:
		return TASK_SCENE_ECHO_REF_DL;
	default:
		pr_warn("%s() err\n", __func__);
		return -1;
	}
	return 0;
}

int get_dspdaiid_by_dspscene(int dspscene)
{
	switch (dspscene) {
	case TASK_SCENE_VOIP:
		return AUDIO_TASK_VOIP_ID;
	case TASK_SCENE_PRIMARY:
		return AUDIO_TASK_PRIMARY_ID;
	case TASK_SCENE_PLAYBACK_MP3:
		return AUDIO_TASK_OFFLOAD_ID;
	case TASK_SCENE_DEEPBUFFER:
		return AUDIO_TASK_DEEPBUFFER_ID;
	case TASK_SCENE_AUDPLAYBACK:
		return AUDIO_TASK_PLAYBACK_ID;
	case TASK_SCENE_CAPTURE_UL1:
		return AUDIO_TASK_CAPTURE_UL1_ID;
	case TASK_SCENE_A2DP:
		return AUDIO_TASK_A2DP_ID;
	case TASK_SCENE_DATAPROVIDER:
		return AUDIO_TASK_DATAPROVIDER_ID;
	case TASK_SCENE_FAST:
		return AUDIO_TASK_FAST_ID;
	case TASK_SCENE_MUSIC:
		return AUDIO_TASK_MUSIC_ID;
	case TASK_SCENE_CALL_FINAL:
		return AUDIO_TASK_CALL_FINAL_ID;
	case TASK_SCENE_KTV:
		return AUDIO_TASK_KTV_ID;
	case TASK_SCENE_CAPTURE_RAW:
		return AUDIO_TASK_CAPTURE_RAW_ID;
	case TASK_SCENE_FM_ADSP:
		return AUDIO_TASK_FM_ADSP_ID;
	case TASK_SCENE_UL_PROCESS:
		return AUDIO_TASK_UL_PROCESS_ID;
	case TASK_SCENE_ECHO_REF_UL:
		return AUDIO_TASK_ECHO_REF_ID;
	case TASK_SCENE_ECHO_REF_DL:
		return AUDIO_TASK_ECHO_REF_DL_ID;
	default:
		pr_info("%s() err dspscene=%d\n", __func__, dspscene);
		return -1;
	}
	return 0;
}

/* todo:: refine for check mechanism.*/
int get_audio_memery_type(struct snd_pcm_substream *substream)
{
	if (substream->runtime->dma_addr < 0x20000000)
		return MEMORY_AUDIO_SRAM;
	else
		return MEMORY_AUDIO_DRAM;
}

int afe_get_pcmdir(int dir, struct audio_hw_buffer buf)
{
	int ret = -1, i = 0, memif = 0;

	if (dir == SNDRV_PCM_STREAM_CAPTURE)
		ret = AUDIO_DSP_TASK_PCM_HWPARAM_UL;
	else if (dir == SNDRV_PCM_STREAM_PLAYBACK)
		ret = AUDIO_DSP_TASK_PCM_HWPARAM_DL;

	if (buf.hw_buffer == BUFFER_TYPE_SHARE_MEM)
		return ret;

	/* check if it is hardware buffer
	 * if yes ==> check if it is ref buffer.
	 */
	memif = buf.audio_memiftype;
	for (i = 0; i < AUDIO_TASK_DAI_NUM; i++) {
		if (get_afememref_by_afe_taskid(i) == memif &&
		   buf.hw_buffer == BUFFER_TYPE_HW_MEM &&
		   get_task_attr(i, ADSP_TASK_ATTR_REF_RUNTIME) == 1) {
			ret = AUDIO_DSP_TASK_PCM_HWPARAM_REF;
			break;
		}
	}

	return ret;
}

int get_dsp_task_attr(int dsp_id, int task_attr)
{
	return get_task_attr(dsp_id, task_attr);
}

int get_dsp_task_id_from_str(const char *task_name)
{
	int ret = -1;

	if (strstr(task_name, "primary"))
		ret = AUDIO_TASK_PRIMARY_ID;
	else if (strstr(task_name, "deepbuffer"))
		ret = AUDIO_TASK_DEEPBUFFER_ID;
	else if (strstr(task_name, "voip"))
		ret = AUDIO_TASK_VOIP_ID;
	else if (strstr(task_name, "playback"))
		ret = AUDIO_TASK_PLAYBACK_ID;
	else if (strstr(task_name, "call_final"))
		ret = AUDIO_TASK_CALL_FINAL_ID;
	else if (strstr(task_name, "ktv"))
		ret = AUDIO_TASK_KTV_ID;
	else if (strstr(task_name, "fm"))
		ret = AUDIO_TASK_FM_ADSP_ID;
	else if (strstr(task_name, "offload"))
		ret = AUDIO_TASK_OFFLOAD_ID;
	else if (strstr(task_name, "capture"))
		ret = AUDIO_TASK_CAPTURE_UL1_ID;
	else if (strstr(task_name, "fast"))
		ret = AUDIO_TASK_FAST_ID;
	else
		pr_info("%s(), %s has no task id, ret %d",
			__func__, task_name, ret);

	return ret;
}

static int set_aud_buf_attr(struct audio_hw_buffer *audio_hwbuf,
			    struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    int irq_usage,
			    struct snd_soc_dai *dai)
{
	int ret = 0;

	ret = set_afe_audio_pcmbuf(audio_hwbuf, substream);
	if (ret < 0) {
		pr_info("set_afe_audio_pcmbuf fail\n");
		return -1;
	}

	ret = set_audiobuffer_hw(audio_hwbuf, BUFFER_TYPE_HW_MEM);
	if (ret < 0) {
		pr_info("set_audiobuffer_hw fail\n");
		return -1;
	}

	ret = set_audiobuffer_audio_irq_num(audio_hwbuf, irq_usage);
	if (ret < 0) {
		pr_info("set_audiobuffer_audio_irq_num fail\n");
		return -1;
	}

	ret = set_audiobuffer_audio_memiftype(audio_hwbuf, dai->id);
	if (ret < 0) {
		pr_info("set_audiobuffer_audio_memiftype fail\n");
		return -1;
	}

	ret = set_audiobuffer_memorytype(
		audio_hwbuf,
		get_audio_memery_type(substream));
	if (ret < 0) {
		pr_info("set_audiobuffer_memorytype fail\n");
		return -1;
	}

	ret = set_audiobuffer_attribute(
		audio_hwbuf,
		substream, params,
		afe_get_pcmdir(substream->stream, *audio_hwbuf));
	if (ret < 0) {
		pr_info("set_audiobuffer_attribute fail\n");
		return -1;
	}

	ret = set_audiobuffer_threshold(audio_hwbuf, substream);
	if (ret < 0) {
		pr_info("set_audiobuffer_threshold fail\n");
		return -1;
	}

	pr_info("%s() memiftype: %d ch: %u fmt: %u rate: %u dir: %d, start_thres: %u stop_thres: %u period_size: %d period_cnt: %d\n",
		__func__,
		audio_hwbuf->audio_memiftype,
		audio_hwbuf->aud_buffer.buffer_attr.channel,
		audio_hwbuf->aud_buffer.buffer_attr.format,
		audio_hwbuf->aud_buffer.buffer_attr.rate,
		audio_hwbuf->aud_buffer.buffer_attr.direction,
		audio_hwbuf->aud_buffer.start_threshold,
		audio_hwbuf->aud_buffer.stop_threshold,
		audio_hwbuf->aud_buffer.period_size,
		audio_hwbuf->aud_buffer.period_count);

	return 0;
}

/* function warp playback buffer information send to dsp */
int afe_pcm_ipi_to_dsp(int command, struct snd_pcm_substream *substream,
		       struct snd_pcm_hw_params *params,
		       struct snd_soc_dai *dai,
		       struct mtk_base_afe *afe)
{
	int task_id = 0, ret = 0;
	struct mtk_base_dsp *dsp = (struct mtk_base_dsp *)local_base_dsp;
	void *ipi_audio_buf; /* dsp <-> audio data struct*/
	struct mtk_base_dsp_mem *dsp_memif;
	struct mtk_base_afe_memif *memif = &afe->memif[dai->id];

	task_id = get_taskid_by_afe_daiid(dai->id);
	if (task_id < 0 || task_id >= AUDIO_TASK_DAI_NUM)
		return -1;

	if (get_task_attr(task_id, ADSP_TASK_ATTR_RUMTIME) <= 0 ||
	    get_task_attr(task_id, ADSP_TASK_ATTR_DEFAULT) <= 0)
		return -1;

	pr_info("%s(), command 0x%x\n", __func__, command);

	dsp_memif = (struct mtk_base_dsp_mem *)&dsp->dsp_mem[task_id];

	/* send msg by task , unsing common function*/
	switch (command) {
	case AUDIO_DSP_TASK_PCM_HWPARAM:
		set_aud_buf_attr(&dsp_memif->audio_afepcm_buf,
				 substream,
				 params,
				 memif->irq_usage,
				 dai);

		/* send audio_afepcm_buf to SCP side*/
		ipi_audio_buf = (void *)
				 dsp_memif->msg_atod_share_buf.va_addr;
		memcpy((void *)ipi_audio_buf,
		       (void *)&dsp_memif->audio_afepcm_buf,
		       sizeof(struct audio_hw_buffer));

#ifdef DEBUG_VERBOSE
		dump_audio_hwbuffer(ipi_audio_buf);
#endif

		/* send to task with hw_param information ,
		 * buffer and pcm attribute
		 */
		ret = mtk_scp_ipi_send(get_dspscene_by_dspdaiid(task_id),
				       AUDIO_IPI_PAYLOAD,
				       AUDIO_IPI_MSG_NEED_ACK,
				       AUDIO_DSP_TASK_PCM_HWPARAM,
				       sizeof(dsp_memif->msg_atod_share_buf.phy_addr),
				       0,
				       (char *)
				       &dsp_memif->msg_atod_share_buf.phy_addr);
		break;
	case AUDIO_DSP_TASK_PCM_PREPARE:
		set_aud_buf_attr(&dsp_memif->audio_afepcm_buf,
				 substream,
				 params,
				 memif->irq_usage,
				 dai);

		/* send audio_afepcm_buf to SCP side*/
		ipi_audio_buf =
			(void *)dsp_memif->msg_atod_share_buf.va_addr;
		memcpy((void *)ipi_audio_buf,
		       (void *)&dsp_memif->audio_afepcm_buf,
		       sizeof(struct audio_hw_buffer));

#ifdef DEBUG_VERBOSE
		dump_audio_hwbuffer(ipi_audio_buf);
#endif

		/* send to task with prepare status*/
		ret = mtk_scp_ipi_send(get_dspscene_by_dspdaiid(task_id),
				       AUDIO_IPI_PAYLOAD,
				       AUDIO_IPI_MSG_NEED_ACK,
				       AUDIO_DSP_TASK_PCM_PREPARE,
				       sizeof(dsp_memif->msg_atod_share_buf.phy_addr),
				       0,
				       (char *)
				       &dsp_memif->msg_atod_share_buf.phy_addr);
		break;
	case AUDIO_DSP_TASK_PCM_HWFREE:
		set_aud_buf_attr(&dsp_memif->audio_afepcm_buf,
				 substream,
				 params,
				 memif->irq_usage,
				 dai);
		/* send to task with prepare status*/
		ret = mtk_scp_ipi_send(get_dspscene_by_dspdaiid(task_id),
				       AUDIO_IPI_MSG_ONLY,
				       AUDIO_IPI_MSG_NEED_ACK,
				       AUDIO_DSP_TASK_PCM_HWFREE,
				       afe_get_pcmdir(substream->stream,
				       dsp_memif->audio_afepcm_buf),
				       0,
				       NULL);
		break;
	default:
		pr_warn("%s error command: %d\n", __func__, command);
		return -1;
	}
	return ret;
}

void mtk_dsp_pcm_ipi_recv(struct ipi_msg_t *ipi_msg)
{
	struct mtk_base_dsp *dsp = get_ipi_recv_private();

	if (ipi_msg == NULL) {
		pr_info("%s ipi_msg == NULL\n", __func__);
		return;
	}

	if (!is_audio_task_dsp_ready(ipi_msg->task_scene)) {
		pr_info("%s(), is_adsp_ready send false\n", __func__);
		return;
	}

	if (dsp->dsp_ipi_ops.ipi_handler)
		dsp->dsp_ipi_ops.ipi_handler(dsp, ipi_msg);
}


#ifdef CONFIG_MTK_AUDIODSP_SUPPORT
int mtk_dsp_register_feature(int id)
{
	return adsp_register_feature(id);
}
#else
int mtk_dsp_register_feature(int id)
{
	int ret = 0;

	scp_register_feature(id);
	return ret;
}
#endif


#ifdef CONFIG_MTK_AUDIODSP_SUPPORT
int mtk_dsp_deregister_feature(int id)
{
	return adsp_deregister_feature(id);
}
#else
int mtk_dsp_deregister_feature(int id)
{
	int ret = 0;

	scp_deregister_feature(id);
	return ret;
}
#endif

#ifdef CONFIG_MTK_AUDIODSP_SUPPORT
static int mtk_audio_dsp_event_receive(
	struct notifier_block *this,
	unsigned long event,
	void *ptr)
{
	switch (event) {
	case ADSP_EVENT_STOP:
		mtk_audio_set_adsp_reset_status(true);
		break;
	case ADSP_EVENT_READY:
		mtk_reinit_adsp();
		break;
	default:
		pr_info("event %lu err", event);
	}
	return 0;
}

static struct notifier_block mtk_audio_dsp_notifier = {
	.notifier_call = mtk_audio_dsp_event_receive,
	.priority = AUDIO_PLAYBACK_FEATURE_PRI,
};
#endif

int mtk_audio_register_notify(void)
{
#ifdef CONFIG_MTK_AUDIODSP_SUPPORT
	adsp_register_notify(&mtk_audio_dsp_notifier);
#endif
	return 0;
}

int mtk_audio_set_adsp_reset_status(int status)
{
	struct mtk_base_dsp *dsp = NULL;

	dsp = get_dsp_base();

	if (dsp == NULL) {
		pr_info("%s dsp == NULL\n", __func__);
		return -1;
	}

	dsp->adsp_reset = status;
	pr_info("%s adsp_reset[%d]\n", __func__, dsp->adsp_reset);

	return 0;
}

/* read and clear*/
bool mtk_audio_get_adsp_reset_status(void)
{
	struct mtk_base_dsp *dsp = NULL;
	bool ret = false;

	dsp = get_dsp_base();

	if (dsp == NULL) {
		pr_info("%s dsp == NULL\n", __func__);
		return -1;
	}

	if (dsp->adsp_reset)
		ret = true;
	else
		ret = false;

	dsp->adsp_reset = false;

	pr_info("%s ret[%d] dsp->adsp_reset[%d]\n", __func__, ret, dsp->adsp_reset);

	return ret;
}

