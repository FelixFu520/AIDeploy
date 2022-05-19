#define TYPE_ORTAPI_API_EXPORTS
#include <windows.h>
#include "ortCore.h"
#include "F_log.h"
#include "ORTAPI.h"


// ���캯��
ORTAPI::ORTAPI()
{
	m_pORTCore = nullptr;
}

// ��������
ORTAPI::~ORTAPI()
{
	if (m_pORTCore != nullptr)
	{
		delete m_pORTCore;
	}
}

// init��ʼ��
std::shared_ptr<ORTCore_ctx> ORTAPI::init(
	const Params& params, 
	int& nErrnoFlag
) {
	// ��ʼ����־����
	YLog ortLog(YLog::INFO, params.log_path, YLog::ADD); 
	ortLog.W(__FILE__, __LINE__, YLog::INFO, "init", "��ʼ��ORTAPI");

	// Ϊ��Ա������ֵ����ʼ��ORTCORE��
	m_pORTCore = new ORTCORE();
	if (m_pORTCore == nullptr) {
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "init", "��ʼ��ʧ��");
		return nullptr;
	}
	
	// ��ʼ��ORTCORE��
	return m_pORTCore->init(params, nErrnoFlag);
}

// Classification
int ORTAPI::classify(
	std::shared_ptr<ORTCore_ctx> ctx, 
	const std::vector<CoreImage*>& vInCoreImages, 
	std::vector<std::vector<ClassifyResult>>& vvOutClsRes
)
{
	// 1. ���ָ���Ƿ�Ϊ��
	if (ctx == nullptr || m_pORTCore == nullptr) {
		return FF_ERROR_PNULL;
	}

	// 2. ��ʼ����־����
	YLog ortLog(YLog::INFO, ctx.get()->params.log_path, YLog::ADD);
	ortLog.W(__FILE__, __LINE__, YLog::INFO, "Classification", "����");

	// 3. ��CoreImageת��Opencv
	std::vector<cv::Mat> input_images;
	for (int i = 0; i < vInCoreImages.size(); i++) {
		cv::Mat cv_img = cv::Mat(
			vInCoreImages[i]->height_, 
			vInCoreImages[i]->width_, 
			CV_8UC(vInCoreImages[i]->channal_), 
			vInCoreImages[i]->imagedata_, 
			vInCoreImages[i]->imagestep_
		).clone();
		input_images.push_back(cv_img);
	}
	
	// 4. ���Ŀ�����
	m_pORTCore->classify(ctx, input_images, vvOutClsRes);
	
	return FF_OK;
}

// Anomaly Detection
int ORTAPI::anomaly(
	std::shared_ptr<ORTCore_ctx> ctx,
	const std::vector<CoreImage*> &vInCoreImages,
	std::vector<CoreImage*>& vOutCoreImages,
	float threshold,
	int pixel_value
)
{
	// 1.���ָ���Ƿ�Ϊ��
	if (ctx == nullptr || m_pORTCore == nullptr) {
		return FF_ERROR_PNULL;
	}

	// 2.��ʼ����־����
	YLog ortLog(YLog::INFO, ctx.get()->params.log_path, YLog::ADD);
	ortLog.W(__FILE__, __LINE__, YLog::INFO, "anomaly", "ORTAPI");

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
	m_pORTCore->anomaly(ctx, input_images, output_images);

	// 5. opencv -> coreimage
	for (int i = 0; i < output_images.size(); i++) {
		cv::threshold(output_images[i], output_images[i], threshold, pixel_value, cv::THRESH_BINARY);
		output_images[i].convertTo(output_images[i], CV_8U);
		//cv::imwrite("E:/test/" + std::string(std::to_string(i)) + ".png", output_images[i]);
	}
	int height = ctx.get()->session.get()->mInputDims[0][2];
	int width = ctx.get()->session.get()->mInputDims[0][3];
	for (int n = 0; n < output_images.size(); n++)
	{
		memcpy_s(vOutCoreImages[n]->imagedata_, width * height, output_images[n].data, width * height);
		vOutCoreImages[n]->channal_ = 1;
		vOutCoreImages[n]->imagestep_ = width;
		vOutCoreImages[n]->height_ = height;
		vOutCoreImages[n]->width_ = width;
	}
	return FF_OK;

}

// Segmentation
int ORTAPI::segment(
	std::shared_ptr<ORTCore_ctx> ctx,
	const std::vector<CoreImage*>& vInCoreImages, 
	std::vector<CoreImage*>& vOutCoreImages
)
{
	// 1. ���ָ���Ƿ�Ϊ��
	if (ctx == nullptr || m_pORTCore == nullptr) {
		return FF_ERROR_PNULL;
	}

	// 2. ��ʼ����־����
	YLog ortLog(YLog::INFO, ctx.get()->params.log_path, YLog::ADD);
	ortLog.W(__FILE__, __LINE__, YLog::INFO, "seg", "�ָ�");

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
	m_pORTCore->segment(ctx, input_images, output_images);

	// 5. opencv -> coreimage
	int batch_size, height, width;
	this->getOutputDimsK(ctx, batch_size, height, width); // ���onnx�е�����ά��
	for (int n = 0; n < output_images.size(); n++)
	{
		output_images[n].convertTo(output_images[n], CV_8U);
		memcpy_s(vOutCoreImages[n]->imagedata_, width * height, output_images[n].data, width * height);
		vOutCoreImages[n]->channal_ = 1;
		vOutCoreImages[n]->imagestep_ = width;
		vOutCoreImages[n]->height_ = height;
		vOutCoreImages[n]->width_ = width;
	}
	return FF_OK;
}

// Object Detection
int ORTAPI::detect(
	std::shared_ptr<ORTCore_ctx> ctx, 
	const std::vector<CoreImage*>& vInCoreImages, 
	std::vector<std::vector<BBox>>& vvOutBBoxs
)
{
	return FF_OK;
}


int ORTAPI::getInputDimsK(
	std::shared_ptr<ORTCore_ctx> ctx, 
	int & nBatch,
	int & nChannels,
	int & nHeight,
	int & nWidth
)
{
	if (ctx == nullptr || m_pORTCore == nullptr) {
		return FF_ERROR_PNULL;
	}
	m_pORTCore->getInputDimsK(ctx, nBatch, nChannels, nHeight, nWidth);
	return FF_OK;
}


int ORTAPI::getOutputDimsK(
	std::shared_ptr<ORTCore_ctx> ctx,
	int & nBatch,
	int & nHeight,
	int & nWidth
)
{
	if (ctx == nullptr || m_pORTCore == nullptr) {
		return FF_ERROR_PNULL;
	}
	m_pORTCore->getOutputDimsK(ctx, nBatch, nHeight, nWidth);
	return FF_OK;
}

int ORTAPI::getOutputDimsK(
	std::shared_ptr<ORTCore_ctx> ctx,
	int & nBatch,
	int & numClass
)
{
	if (ctx == nullptr || m_pORTCore == nullptr) {
		return FF_ERROR_PNULL;
	}
	m_pORTCore->getOutputDimsK(ctx, nBatch, numClass);
	return FF_OK;
}

