/*****************************************************************************
* @author : FelixFu
* @date : 2021/10/10 14:40
* @last change :
* @description : TRTCORE ���API����TRTCORE�Ͻ�һ����װ��ȥ����opencv������
*****************************************************************************/
#ifndef TRTAPI_H
#define TRTAPI_H

#if defined (TYPE_TRTAPI_API_EXPORTS)
#define TYPE_TRTAPI_API __declspec(dllexport)
#else
#define TYPE_TRTAPI_API __declspec(dllimport)
#endif

#include <iostream>
#include <string>
#include <vector>

#include "params.h"
#include "loguru.hpp" // https://github.com/emilk/loguru


// \! ͼ��Ļ�������
class CoreImage
{
public:
	void SetValue(int channal, int width, int height, int step, unsigned char* data)
	{
		channal_ = channal;
		width_ = width;
		height_ = height;
		imagestep_ = step;
		imagedata_ = data;
	};

public:
	//ͼ��ͨ��
	int channal_;
	//��	
	int width_;
	//��
	int height_;
	//ÿ���ֽ���
	int imagestep_;
	//ͼ������
	unsigned char *imagedata_;
};

// \! API�ӿ�
class TRTCORE;  // ������ӿ�
class TYPE_TRTAPI_API TRTAPI { // Ӧ�ò�ӿ�
public:
	typedef struct TRTCore_ctx TRTCore_ctx;   //������Ҫ�õ���һ���ṹ�壬������params��engine��pool��
	TRTAPI();	// ���캯��
	~TRTAPI();	// ��������
				
	// \! ��ʼ��
	// \@param:params     ��ʼ������
	// \@param:nErrnoFlag ��ʼ�������룬�����params.h
	std::shared_ptr<TRTCore_ctx> init(const Params& params, int& nErrnoFlag); 	

	// \! ����
	// \@param ctx:ִ��������
	// \@param vInCoreImages:����ͼ���б�CoreImage��ʽ
	// \@param vvOutClsRes:��������ClassifyResult��ʽ
	int classify(std::shared_ptr<TRTCore_ctx> ctx,const std::vector<CoreImage*> &vInCoreImages,	std::vector<std::vector<ClassifyResult>>& vvOutClsRes);

	// \! �ָ�
	// \@param ctx: ִ��������
	// \@param vInCoreImages: ����ͼƬvector��CoreImage
	// \@param vOutCoreImages:���ͼƬvector��CoreImage
	// \@param verbose: ���Ϊtrue,return the probability graph, else return the class id image
	int segment(std::shared_ptr<TRTCore_ctx> ctx, const std::vector<CoreImage*> &vInCoreImages, std::vector<CoreImage*>& vOutCoreImages,bool verbose=false);

	// \! Ŀ����
	// \@param ctx:ִ��������
	// \@param vInCoreImages:����ͼƬ���飬CoreImage
	// \@param vvOutBBoxs:���������飬BBox
	int detect(std::shared_ptr<TRTCore_ctx> ctx,const std::vector<CoreImage*> &vInCoreImages, std::vector<std::vector<BBox>>& vvOutBBoxs);

	// \! �쳣���
	// \@param ctx:ִ��������
	// \@param vInCoreImages:����ͼƬ�б�CoreImage
	// \@param vOutCoreImages:���ͼƬ���飬CoreImage
	// \@param threshold:��ֵ
	// \@param maxValue:���ֵ����һ��ʱʹ��
	// \@param minValue:��Сֵ����һ��ʱʹ��
	// \@param pixel_value:��ֵ��ͼ���ֵ����һ��ʱʹ��
	int anomaly(std::shared_ptr<TRTCore_ctx> ctx,const std::vector<CoreImage*> &vInCoreImages,std::vector<CoreImage*>& vOutCoreImages,const float threshold,const float maxValue= 27.811206817626953,const float minValue= 1.6174373626708984,const int pixel_value=255);
	
	// \! ��ȡ�Կ�����
	// \@param ctx:ִ��������
	// \@param number:gpu����
	int getNumberGPU(std::shared_ptr<TRTCore_ctx> ctx,int& number);

	// \! ��ȡ����ά��
	// \@param ctx:ִ��������
	// \@param nBatch:batchsize
	// \@param nChannels:channels
	// \@param nHeight:height
	// \@param nWidth:width
	// \@param index:��index�����룬����onnx�ж�����룬��ͨ��index��ָ��
	int getInputDims(std::shared_ptr<TRTCore_ctx> ctx,int& nBatch, int& nChannels, int& nHeight, int& nWidth,int index=0);

	// \! ��ȡ���ά��
	// \@param ctx��ִ��������
	// \@param nBatch:batchsize
	// \@param nHeight:Height
	// \@param nWidth:Width
	// \@param index:��index�����������onnx�ж���������ͨ��index��ָ��
	int getOutputDims(std::shared_ptr<TRTCore_ctx> ctx,int& nBatch,int& nHeight,int& nWidth,int index=0);

private:
	TRTCORE *m_pTRTCore; // Ϊ�˷��������������������������ӿڣ�������ӿڡ�Ӧ�ò�ӿڡ�Ӧ�ò�ӿ����ڿ�����֮�Ϸ�װ�Ķ���ӿڡ�
};
#endif