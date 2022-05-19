/*****************************************************************************
* @author : FelixFu
* @date : 2021/10/10 14:40
* @last change :
* @description : ONNXRuntime ���API����ORTCore�Ͻ�һ����װ��ȥ����opencv������
*****************************************************************************/
#ifndef ORTAPI_H
#define ORTAPI_H

#if defined (TYPE_ORTAPI_API_EXPORTS)
#define TYPE_ORTAPI_API __declspec(dllexport)
#else
#define TYPE_ORTAPI_API __declspec(dllimport)
#endif

#include <iostream>
#include <string>
#include <vector>

#include "params.h"


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
class ORTCORE;  // ���Ĳ�ӿ�
class TYPE_ORTAPI_API ORTAPI { // API�ӿ�
public:
	typedef struct ORTCore_ctx ORTCore_ctx;   //������Ҫ�õ���һ���ṹ�壬������params��session��
	ORTAPI();
	~ORTAPI();

	// \! ��ʼ��
	std::shared_ptr<ORTCore_ctx> init(
		const Params& params,	// @param:params     ��ʼ������
		int& nErrnoFlag			// @param:nErrnoFlag ��ʼ�������룬�����params.h
	); 	

	// \! ����
	int classify(
		std::shared_ptr<ORTCore_ctx> ctx, 
		const std::vector<CoreImage*> &vInCoreImages, 
		std::vector<std::vector<ClassifyResult>>& vvOutClsRes
	);

	// \! �쳣���
	int anomaly(
		std::shared_ptr<ORTCore_ctx> ctx, 
		const std::vector<CoreImage*> &vInCoreImages, 
		std::vector<CoreImage*>& vOutCoreImages,
		float threshold,
		int pixel_value
	);
	
	// \! �ָ�
	int segment(
		std::shared_ptr<ORTCore_ctx> ctx, 
		const std::vector<CoreImage*> &vInCoreImages, 
		std::vector<CoreImage*>& vOutCoreImages
	);

	// \! Ŀ��������
	int detect(
		std::shared_ptr<ORTCore_ctx> ctx, 
		const std::vector<CoreImage*> &vInCoreImages, 
		std::vector<std::vector<BBox>>& vvOutBBoxs
	);

	// \! ���InputDims
	int getInputDimsK(
		std::shared_ptr<ORTCore_ctx> ctx, 
		int& nBatch,
		int& nChannels,
		int& nHeight, 
		int &nWidth
	);

	// \! ���OutputDims
	int getOutputDimsK(
		std::shared_ptr<ORTCore_ctx> ctx,
		int& nBatch,
		int& nHeight,
		int &nWidth
	);

	// \! ���BatchSize, NumClass
	int getOutputDimsK(
		std::shared_ptr<ORTCore_ctx> ctx,
		int& nBatch,
		int &numClass
	);

private:
	ORTCORE *m_pORTCore; // Ϊ�˷��������������������������ӿڣ�������ӿڡ�Ӧ�ò�ӿڡ�Ӧ�ò�ӿ����ڿ�����֮�Ϸ�װ�Ķ���ӿڡ�
};
#endif