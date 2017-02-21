#include "common.h"

/*************************************************
	Function:		hello
	Description:	���������д���Ĳ���
	Calls:			��
	Called By:		main
	Input:			(in)argc : Ĭ�������в���
					(in)argv : Ĭ�������в���					
	Output:			(out)io_param : ���������еĽ��
	Return:			true : �����н�����ȷ
					false : �����н�������
*************************************************/
static bool hello(int argc, char **argv, IOFiles &files)
{
	printf("FFMpeg Scaling Demo.\nCommand format: %s input_file input_frame_size output_file output_frame_size\n", argv[0]);
	if (argc != 5)
	{
		printf("Error: command line error, please re-check.\n");
		return false;
	}

	files.inputName = argv[1];
	files.inputFrameSize = argv[2];
	files.outputName = argv[3];
	files.outputFrameSize = argv[4];

	fopen_s(&files.iFile, files.inputName, "rb+");
	if (!files.iFile)
	{
		printf("Error: cannot open input file.\n");
		return false;
	}

	fopen_s(&files.oFile, files.outputName, "wb+");
	if (!files.oFile)
	{
		printf("Error: cannot open output file.\n");
		return false;
	}

	return true;
}

/*************************************************
	Function:		read_yuv_from_ifile
	Description:	�������ļ��ж�ȡ��������
	Calls:			��
	Called By:		main
	Input:			(in)srcWidth : ����ͼ��Ŀ��
					(in)srcHeight : ����ͼ��ĵĸ߶�
					(in)color_plane ����ɫ������0����Y;1����U;2����V
					(in)files : ���������ļ��Ľṹ
	Output:			(out)src_data : �����������ݵĻ���
					(out)src_linesize ��
	Return:			true : �����н�����ȷ
					false : �����н�������
*************************************************/
static int read_yuv_from_ifile(uint8_t *src_data[4], int src_linesize[4], int srcWidth, int srcHeight, int color_plane,IOFiles &files)
{
	int frame_height	= color_plane == 0 ? srcHeight : srcHeight / 2;
	int frame_width		= color_plane == 0 ? srcWidth  : srcWidth  / 2;
	int frame_size		= frame_width * frame_height;
	int frame_stride	= src_linesize[color_plane];

	if (frame_width == frame_stride)
	{
		// ��ȺͿ����ȣ�������Ϣ�������
		fread_s(src_data[color_plane], frame_size, 1, frame_size, files.iFile);
	} 
	else
	{
		// ���С�ڿ�ȣ�������Ϣ����ռ�֮����ڼ��
		for (int row_idx = 0; row_idx < frame_height; row_idx++)
		{
			fread_s(src_data[color_plane] + row_idx * frame_stride, frame_width, 1, frame_width, files.iFile);
		}
	}

	return frame_size;
}

/*************************************************
Function:		main
Description:	��Ƶ����
Test: ffplay -f rawvideo -video_size 640X480 -autoexit -i ../video/vsca.yuv
*************************************************/
#define MAX_FRAME_NUM 100

int main(int argc, char **argv)
{
	int ret = 0;

	// �����������������
	IOFiles files = { NULL };

	uint8_t *src_data[4], *dst_data[4];
	struct SwsContext *sws_ctx = NULL;

	do {
		if (!hello(argc, argv, files))
		{
			break;
		}
		int srcWidth, srcHeight, dstWidth, dstHeight;
		if (av_parse_video_size(&srcWidth, &srcHeight, files.inputFrameSize))
		{
			printf("Error: parsing input size failed.\n");
			break;
		}
		if (av_parse_video_size(&dstWidth, &dstHeight, files.outputFrameSize))
		{
			printf("Error: parsing output size failed.\n");
			break;
		}

		// ����SwsContext�ṹ
		enum AVPixelFormat src_pix_fmt = AV_PIX_FMT_YUV420P;
		enum AVPixelFormat dst_pix_fmt = AV_PIX_FMT_YUV420P;
		sws_ctx = sws_getContext(srcWidth, srcHeight, src_pix_fmt, dstWidth, dstHeight, dst_pix_fmt, SWS_BILINEAR, NULL, NULL, NULL);
		if (!sws_ctx)
		{
			printf("Error: allocating SwsContext struct failed.\n");
			break;
		}

		// ����input��output
		int src_linesize[4], dst_linesize[4];
		if ((ret = av_image_alloc(src_data, src_linesize, srcWidth, srcHeight, src_pix_fmt, 32)) < 0)
		{
			printf("Error: allocating src image failed.\n");
			break;
		}
		if ((ret = av_image_alloc(dst_data, dst_linesize, dstWidth, dstHeight, dst_pix_fmt, 1)) < 0)
		{
			printf("Error: allocating dst image failed.\n");
			break;
		}

		// �����frame��д��������ļ�
		int dst_bufsize = ret;
		for (int idx = 0; idx < MAX_FRAME_NUM; idx++)
		{
			read_yuv_from_ifile(src_data, src_linesize, srcWidth, srcHeight, 0, files);
			read_yuv_from_ifile(src_data, src_linesize, srcWidth, srcHeight, 1, files);
			read_yuv_from_ifile(src_data, src_linesize, srcWidth, srcHeight, 2, files);

			// ��Ƶ��������
			sws_scale(sws_ctx, (const uint8_t * const*)src_data, src_linesize, 0, srcHeight, dst_data, dst_linesize);

			fwrite(dst_data[0], 1, dst_bufsize, files.oFile);
		}

		printf("Video scaling succeeded.\n");

	} while (false);
    
	// ��β���ͷ��ڴ�
	fclose(files.iFile);
	fclose(files.oFile);
	av_freep(&src_data[0]);
	av_freep(&dst_data[0]);
	sws_freeContext(sws_ctx);

	return 0;
}