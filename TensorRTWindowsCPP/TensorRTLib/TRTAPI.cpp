#define TYPE_TRTAPI_API_EXPORTS
#include <windows.h>

#include "TRTAPI.h"
#include "trtCore.h"

// \! ���캯��
TRTAPI::TRTAPI()
{
	m_pTRTCore = nullptr;
}

// \! ��������
TRTAPI::~TRTAPI()
{
	if (m_pTRTCore != nullptr)
	{
		delete m_pTRTCore;
	}
}

// \! ��ʼ��
// \@param:params     ��ʼ������
// \@param:nErrnoFlag ��ʼ�������룬�����params.h
std::shared_ptr<TRTCore_ctx> TRTAPI::init(const Params& params, int& nErrnoFlag)
{
	// 1. ������־
	loguru::add_file(params.log_path.c_str(), loguru::Append, loguru::Verbosity_MAX);

	// 2. ��ʼ��һ��TRTCOREʵ��
	m_pTRTCore = new TRTCORE();	
	if (m_pTRTCore == nullptr) {
		LOG_F(INFO, "new TRTCORE() Error!!!!");
		return nullptr;
	}

	// 3. ��ʼ��m_pTRTCore
	return m_pTRTCore->init(params, nErrnoFlag);
}

// \! ����
// \@param ctx:ִ��������
// \@param vInCoreImages:����ͼ���б�CoreImage��ʽ
// \@param vvOutClsRes:��������ClassifyResult��ʽ
int TRTAPI::classify(std::shared_ptr<TRTCore_ctx> ctx,const std::vector<CoreImage*>& vInCoreImages,std::vector<std::vector<ClassifyResult>>& vvOutClsRes)
{
	LOG_F(INFO, "classify start ......");

	// 1. ��ָ����
	if (ctx == nullptr || m_pTRTCore == nullptr) {
		LOG_F(INFO, "Init failed, can't call classify");
		return LY_UNKNOW_ERROR;
	}
	
	// 2. ��CoreImageת��Opencv
	std::vector<cv::Mat> input_images;
	for (int i = 0; i < vInCoreImages.size(); i++) {
		cv::Mat cv_img = cv::Mat(
			vInCoreImages[i]->height_, 
			vInCoreImages[i]->width_, 
			CV_8UC(vInCoreImages[i]->channal_), 
			vInCoreImages[i]->imagedata_, 
			vInCoreImages[i]->imagestep_
		).clone();// �˴�ΪʲôҪ��clone��--����Ϊ�粻��clone���뿪���������tmp�����ͷţ�input_images����������
		input_images.push_back(cv_img);
	}

	// 3.����
	m_pTRTCore->classify(ctx, input_images, vvOutClsRes);

	return LY_OK;
}

// \! �ָ�
// \@param ctx: ִ��������
// \@param vInCoreImages: ����ͼƬvector��CoreImage
// \@param vOutCoreImages:���ͼƬvector��CoreImage
int TRTAPI::segment(std::shared_ptr<TRTCore_ctx> ctx, const std::vector<CoreImage*>& vInCoreImages, std::vector<CoreImage*>& vOutCoreImages, bool verbose)
{
	LOG_F(INFO, "segment start ......");

	// 1. ָ���ж�
	if (ctx == nullptr || m_pTRTCore == nullptr) {
		LOG_F(INFO, "Init failed, can't call segment");
		return LY_UNKNOW_ERROR;
	}

	// 2. coreImage -> cvImage
	std::vector<cv::Mat> input_images, output_images;
	for (int i = 0; i < vInCoreImages.size(); i++) {
		cv::Mat cv_img = cv::Mat(vInCoreImages[i]->height_, vInCoreImages[i]->width_, CV_8UC(vInCoreImages[i]->channal_), vInCoreImages[i]->imagedata_, vInCoreImages[i]->imagestep_).clone();
		input_images.push_back(cv_img);
	}

	// 3. TRTCOREִ�зָ����
	m_pTRTCore->segment(ctx, input_images, output_images, verbose);

	// 4. coreImage->opencv image
	int engine_output_batch, engine_output_height, engine_output_width;
	this->getOutputDims(ctx, engine_output_batch, engine_output_height, engine_output_width);//������ά����Ϣ
	for (int n = 0; n < output_images.size(); n++)
	{
		output_images[n].convertTo(output_images[n], CV_8U);//onnx�������float32����main��������8U����
		memcpy_s(vOutCoreImages[n]->imagedata_, engine_output_width * engine_output_height, output_images[n].data, engine_output_width * engine_output_height);
		vOutCoreImages[n]->channal_ = 1;
		vOutCoreImages[n]->imagestep_ = engine_output_width;
		vOutCoreImages[n]->height_ = engine_output_height;
		vOutCoreImages[n]->width_ = engine_output_width;
	}
	return LY_OK;
}

// \! Ŀ����
// \@param ctx:ִ��������
// \@param vInCoreImages:����ͼƬ���飬CoreImage
// \@param vvOutBBoxs:���������飬BBox
int TRTAPI::detect(std::shared_ptr<TRTCore_ctx> ctx, const std::vector<CoreImage*>& vInCoreImages,std::vector<std::vector<BBox>>& vvOutBBoxs)
{
	LOG_F(INFO, "detect start ......");

	return LY_UNKNOW_ERROR;
}

// \! �쳣���
// \@param ctx:ִ��������
// \@param vInCoreImages:����ͼƬ�б�CoreImage
// \@param vOutCoreImages:���ͼƬ���飬CoreImage
// \@param threshold:��ֵ
// \@param maxValue:���ֵ����һ��ʱʹ��
// \@param minValue:��Сֵ����һ��ʱʹ��
// \@param pixel_value:��ֵ��ͼ���ֵ����һ��ʱʹ��
int TRTAPI::anomaly(std::shared_ptr<TRTCore_ctx> ctx,const std::vector<CoreImage*> &vInCoreImages,std::vector<CoreImage*>& vOutCoreImages, const float threshold, const float maxValue,	const float minValue,const int pixel_value) 
{
	LOG_F(INFO, "anomaly start ......");

	// 1.���ָ���Ƿ�Ϊ��
	if (ctx == nullptr || m_pTRTCore == nullptr) {
		LOG_F(INFO, "ָ��Ϊ��");
		return LY_UNKNOW_ERROR;
	}

	// 3.��CoreImageת��Opencv
	std::vector<cv::Mat> input_images, output_images;
	for (int i = 0; i < vInCoreImages.size(); i++) {
		cv::Mat cv_img = cv::Mat(
			vInCoreImages[i]->height_,
			vInCoreImages[i]->width_,
			CV_8UC(vInCoreImages[i]->channal_),
			vInCoreImages[i]->imagedata_,
			vInCoreImages[i]->imagestep_).clone();
		input_images.push_back(cv_img);
	}

	// 4.���Ŀ�����
	m_pTRTCore->anomaly(ctx, input_images, output_images);

	// 5. opencv -> coreimage
	int input_batch, input_channels,input_height, input_width;
	m_pTRTCore->getInputDims(ctx, input_batch, input_channels, input_height, input_width);
	int output_batch, output_height, output_width;
	m_pTRTCore->getOutputDims(ctx, output_batch, output_height, output_width);
	for (int i = 0; i < output_images.size(); i++) {
		cv::resize(output_images[i], output_images[i], cv::Size(input_height, input_width), cv::INTER_LINEAR);
		output_images[i] = (output_images[i] - minValue) / (maxValue - minValue);
		cv::threshold(output_images[i], output_images[i], threshold, pixel_value, cv::THRESH_BINARY);
		output_images[i].convertTo(output_images[i], CV_8U);
		//cv::imwrite("E:/test/" + std::string(std::to_string(i)) + ".png", output_images[i]);
	}
	for (int n = 0; n < output_images.size(); n++)
	{
		memcpy_s(vOutCoreImages[n]->imagedata_, input_width * input_height, output_images[n].data, input_width * input_height);
		vOutCoreImages[n]->channal_ = 1;
		vOutCoreImages[n]->imagestep_ = input_width;
		vOutCoreImages[n]->height_ = input_height;
		vOutCoreImages[n]->width_ = input_width;
	}
	return LY_OK;
}

// \! ��ȡ�Կ�����
// \@param ctx:ִ��������
// \@param number:gpu����
int TRTAPI::getNumberGPU(std::shared_ptr<TRTCore_ctx> ctx,int& number)
{
	return this->m_pTRTCore->getNumberGPU(ctx, number);
}

// \! ��ȡ����ά��
// \@param ctx:ִ��������
// \@param nBatch:batchsize
// \@param nChannels:channels
// \@param nHeight:height
// \@param nWidth:width
// \@param index:��index�����룬����onnx�ж�����룬��ͨ��index��ָ��
int TRTAPI::getInputDims(std::shared_ptr<TRTCore_ctx> ctx, int & nBatch, int & nChannels, int & nHeight, int & nWidth,	int index)
{
	if (ctx == nullptr || m_pTRTCore == nullptr) {
		LOG_F(INFO, "init failed, can't call getDims");
		return LY_UNKNOW_ERROR;
	}
	m_pTRTCore->getInputDims(ctx, nBatch, nChannels, nHeight, nWidth, index);
	return LY_OK;
}

// \! ��ȡ���ά��
// \@param ctx��ִ��������
// \@param nBatch:batchsize
// \@param nHeight:Height
// \@param nWidth:Width
// \@param index:��index�����������onnx�ж���������ͨ��index��ָ��
int TRTAPI::getOutputDims(std::shared_ptr<TRTCore_ctx> ctx,int& nBatch,int& nHeight,int &nWidth,int index)
{
	if (ctx == nullptr || m_pTRTCore == nullptr) {
		LOG_F(INFO, "init failed, can't call getDims");
		return LY_UNKNOW_ERROR;
	}
	m_pTRTCore->getOutputDims(ctx, nBatch, nHeight, nWidth, index);
	return LY_OK;
}