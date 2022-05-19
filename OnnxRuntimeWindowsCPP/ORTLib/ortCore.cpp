#include "ortCore.h"
#include "engine.h"
#include "F_log.h"
#include "utils.h"
using namespace std;

// \! ��ʼ��ORTCore_ctxִ��������
std::shared_ptr<ORTCore_ctx> ORTCORE::init(
	const Params& params, 
	int& nErrnoFlag
) {
	// 1. ��ʼ����־��־�ļ�
	YLog ortLog(YLog::INFO, params.log_path, YLog::ADD);
	try {
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "ORTCORE", "Current version: 2022-05-17");

		// 1.ʹ��ONNXģ���ļ�����ORTSession��
		std::shared_ptr <ORTSession> ortSession(new ORTSession(params, nErrnoFlag));
		if (nErrnoFlag != FF_OK) {
			ortLog.W(__FILE__, __LINE__, YLog::INFO, "ORTCORE", "Can't load onnx file");
			return nullptr;
		}

		// 2. ͨ��ortSession����ORTCore_ctx������
		std::shared_ptr<ORTCore_ctx> ctx(new ORTCore_ctx{ params, ortSession });
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "ORTCORE", "Init Successfully !!!!");
		return ctx;
	}
	catch (const std::invalid_argument& ex) {
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "ORTCORE", "Init failed !!!!");
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "ORTCORE", std::string(ex.what()));
		nErrnoFlag = FF_UNKNOW_ERROR;
		return nullptr;
	}
}


// \! ����
int ORTCORE::classify(
	std::shared_ptr<ORTCore_ctx> ctx, 
	const std::vector<cv::Mat> &cvImages, 
	std::vector<std::vector<ClassifyResult>>& outputs
)
{
	YLog ortLog(YLog::INFO, ctx.get()->params.log_path, YLog::ADD);
	// 1. ctx�Ƿ��ʼ���ɹ�
	if (ctx == nullptr) {
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "Classification", "Init Failed��Can't call classify function !");
		return FF_ERROR_PNULL;
	}

	// 2. �ж�NetType�Ƿ���ȷ
	if (ctx.get()->params.netType != FF_CLS)
	{
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "Classification", "Illegal calls��please check your NetWorkType");
		return FF_ERROR_NETWORK;
	}

	// 3. session��Ϣ��������Ϣ�Ա�
	std::vector<int64_t> inputDims = ctx.get()->session->mInputDims[0];		// ��һ������ά�ȣ�session��
	std::vector<int64_t> outputDims = ctx.get()->session->mOutputDims[0];	// ��һ�����ά�ȣ�session��
	auto input_name = ctx.get()->session->mInputTensorNames[0];				// ��һ����������
	auto output_name = ctx.get()->session->mOutputTensorNames[0];			// ��һ���������
	// ���session��Ϣ, input��output
	int session_batch{ 0 }, session_channels{ 0 }, session_height{ 0 }, session_width{ 0 };
	int session_numClass{ 0 };
	session_batch = inputDims[0];	// ���batchsize��session��
	session_channels = inputDims[1];// ���channels��session��
	session_height = inputDims[2];// ���height��session��
	session_width = inputDims[3];// ���width��session��
	session_numClass = outputDims[1]; // ���number class
	std::vector<const char*> input_node_names;
	std::vector<const char*> output_node_names;
	output_node_names.push_back(ctx.get()->session->mOutputTensorNames[0].c_str());	// ����������
	input_node_names.push_back(ctx.get()->session->mInputTensorNames[0].c_str());	// �����������
	// session��Ϣ��������Ϣ�Ա�
	std::vector<cv::Mat> imgs;	// ���resize���ͼƬ�����ж�ʱ���洢��imgs��
	// 3.1 �ж�batchsize
	if (cvImages.size() != session_batch) {// batchsize�ж�
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "Classification", "����ͼƬ�����batchsize��ONNX batchsize��ͬ ");
		return FF_ERROR_INPUT;
	}
	// 3.2 �ж�channels��HW
	for (int i = 0; i < cvImages.size(); i++) {// c,h,w�ж�
		cv::Mat cv_img = cvImages[i].clone();
		if (cv_img.channels() != session_channels) {
			ortLog.W(__FILE__, __LINE__, YLog::INFO, "Classification", "����ͼƬ��ͨ������session���� ");
			return FF_ERROR_INPUT;
		}
		if (session_height != cv_img.cols || session_width != cv_img.rows) {
			ortLog.W(__FILE__, __LINE__, YLog::INFO, "Classification", "�����ͼƬ�ߴ���session�����,�Զ�resize");
			cv::resize(cv_img, cv_img, cv::Size(session_height, session_width), 0, 0, cv::INTER_LINEAR);
		}

		imgs.push_back(cv_img);
	}
	if (imgs.empty()) {
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "Classification", "No images, please check");
		return FF_ERROR_INPUT;
	}
	
	// 4. Ԥ����ͼƬ
	size_t input_tensor_size = session_batch * session_channels * session_height * session_width;  // ���ͼƬ�ߴ�
	std::vector<float> input_tensor_values(input_tensor_size);	// �������ͼƬ
	auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);// create input tensor object from data values
	normalization(imgs, input_tensor_values, ctx, memory_info);// ��һ��ͼƬ�� ������洢��input_tensor_values
	Ort::Value input_tensor = Ort::Value::CreateTensor<float>(// ͨ��input_tensor_values����input_tensor
		memory_info,
		input_tensor_values.data(),
		input_tensor_size,
		ctx.get()->session->mInputDims[0].data(),
		ctx.get()->session.get()->m_Session.GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape().size());
	assert(input_tensor.IsTensor());	// �ж�input_tensor�Ƿ���Tensor
	
	// 5. ort_inputs����
	std::vector<Ort::Value> ort_inputs;
	ort_inputs.push_back(std::move(input_tensor));

	// 6. infer�������
	auto output_tensors = ctx.get()->session.get()->m_Session.Run(
		Ort::RunOptions{ nullptr },
		input_node_names.data(),
		ort_inputs.data(),
		ort_inputs.size(),
		output_node_names.data(),
		ctx.get()->session.get()->mOutputTensorNames.size());
	assert(output_tensors.size() == 1 && output_tensors.front().IsTensor());

	// 7. ����
	// Get pointer to output tensor float values
	float* floatarr = output_tensors.front().GetTensorMutableData<float>();
	clsPost(floatarr, outputs, session_batch, session_numClass);
	return FF_OK;
}


// \!�쳣���
int ORTCORE::anomaly(
	std::shared_ptr<ORTCore_ctx> ctx, // ִ��������
	const std::vector<cv::Mat> &cvImages, // ����ͼƬ
	std::vector<cv::Mat>& outputs// ���ͼƬ
) 
{
	YLog ortLog(YLog::INFO, ctx.get()->params.log_path, YLog::ADD);//��־����
	// 1. ctx�Ƿ��ʼ���ɹ�
	if (ctx == nullptr) {
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "anomaly", "ctx������");
		return FF_ERROR_PNULL;
	}

	// 2. �ж�NetType�Ƿ���ȷ
	if (ctx.get()->params.netType != FF_ANOMALY)
	{
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "Anomaly", "Illegal calls��please check your NetWorkType");
		return FF_ERROR_NETWORK;
	}

	// 3. session��Ϣ��������Ϣ�Ա�
	std::vector<int64_t> inputDims = ctx.get()->session->mInputDims[0];		// ���onnx�е�һ������ά�ȣ�session��
	std::vector<int64_t> outputDims = ctx.get()->session->mOutputDims[0];	// ���onnx�е�һ�����ά�ȣ�session��
	auto input_name = ctx.get()->session->mInputTensorNames[0];				// ���onnx�е�һ����������
	auto output_name = ctx.get()->session->mOutputTensorNames[0];			// ���onnx�е�һ���������
	// 3.1 �������ά��
	int session_batch{ 0 }, session_channels{ 0 }, session_height{ 0 }, session_width{ 0 };
	session_batch = inputDims[0];	// ���batchsize��session��
	session_channels = inputDims[1];// ���channels��session��
	session_height = inputDims[2];// ���height��session��
	session_width = inputDims[3];// ���width��session��
	// 3.2 ���input��output����
	std::vector<const char*> input_node_names;
	std::vector<const char*> output_node_names;
	output_node_names.push_back(output_name.c_str());	// ����������
	input_node_names.push_back(input_name.c_str());	// �����������
	// 3.3 onnx��batchsize�������������ݵ�batchsize�Ա�
	std::vector<cv::Mat> imgs;	// ���resize���ͼƬ�����ж�ʱ���洢��imgs��
	if (cvImages.size() != session_batch) {// batchsize�ж�
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "Anomaly", "����ͼƬ�����batchsize��ONNX batchsize��ͬ ");
		return FF_ERROR_INPUT;
	}
	// 3.4 onnx��c,h,w�������ͼƬ�Ա�
	for (int i = 0; i < cvImages.size(); i++) {// c,h,w�ж�
		cv::Mat cv_img = cvImages[i].clone();
		if (cv_img.channels() != session_channels) {// ͨ�����ж�
			ortLog.W(__FILE__, __LINE__, YLog::INFO, "Anomaly", "����ͼƬ��ͨ������session���� ");
			return FF_ERROR_INPUT;
		}
		if (session_height != cv_img.cols || session_width != cv_img.rows) {// h��w�ж�
			ortLog.W(__FILE__, __LINE__, YLog::INFO, "Anomaly", "�����ͼƬ�ߴ���session�����,�Զ�resize");
			//cv::resize(cv_img, cv_img, cv::Size(session_height, session_width), 0.5, 0.5, cv::INTER_AREA); // ʹ�ô˷�����onnx��python��ʱ���Ӧ
			cv::resize(cv_img, cv_img, cv::Size(session_height, session_width), cv::INTER_LINEAR); // ʹ�ô˷�����onnx��python��ʱ���Ӧ
		}
		imgs.push_back(cv_img);
	}
	// 3.5 �ж�ͼƬ�Ƿ�Ϊ��
	if (imgs.empty()) {
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "Anomaly", "No images, please check");
		return FF_ERROR_INPUT;
	}

	// 4. Ԥ����ͼƬ
	size_t input_tensor_size = session_batch * session_channels * session_height * session_width;  // ���ͼƬ�ߴ�
	std::vector<float> input_tensor_values(input_tensor_size);	// �������ͼƬ
	auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);// create input tensor object from data values
	normalization(imgs, input_tensor_values, ctx, memory_info);// ��һ��ͼƬ�� ������洢��input_tensor_values
	Ort::Value input_tensor = Ort::Value::CreateTensor<float>(// ͨ��input_tensor_values����input_tensor
		memory_info,// ort�ڴ�����
		input_tensor_values.data(),// ��������
		input_tensor_size,// ���ݴ�С
		ctx.get()->session->mInputDims[0].data(),// onnx�����shape
		ctx.get()->session.get()->m_Session.GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape().size());
	assert(input_tensor.IsTensor());	// �ж�input_tensor�Ƿ���Tensor
	// 5.ort_inputs����
	std::vector<Ort::Value> ort_inputs;
	ort_inputs.push_back(std::move(input_tensor));

	// 6. infer�������
	auto output_tensors = ctx.get()->session.get()->m_Session.Run(
		Ort::RunOptions{ nullptr },
		input_node_names.data(),
		ort_inputs.data(),
		ort_inputs.size(),
		output_node_names.data(),
		ctx.get()->session.get()->mOutputTensorNames.size());
	assert(output_tensors.size() == 1 && output_tensors.front().IsTensor());

	// 7. ����
	// Get pointer to output tensor float values
	float* floatarr = output_tensors.front().GetTensorMutableData<float>();
	anomalyPost(floatarr, outputs, outputDims[0], outputDims[1], outputDims[2]);

	return FF_OK;
}


// \! Segementation
int ORTCORE::segment(
	std::shared_ptr<ORTCore_ctx> ctx, 
	const std::vector<cv::Mat> &cvImages, 
	std::vector<cv::Mat>& outputs
)
{
	YLog ortLog(YLog::INFO, ctx.get()->params.log_path, YLog::ADD);//��־����
	// 1. ctx�Ƿ��ʼ���ɹ�
	if (ctx == nullptr) {
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "seg", "ctx������");
		return FF_ERROR_PNULL;
	}

	// 2. �ж�NetType�Ƿ���ȷ
	if (ctx.get()->params.netType != FF_SEG)
	{
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "seg", "Illegal calls��please check your NetWorkType");
		return FF_ERROR_NETWORK;
	}

	// 3. session��Ϣ��������Ϣ�Ա�
	std::vector<int64_t> inputDims = ctx.get()->session->mInputDims[0];		// ���onnx�е�һ������ά�ȣ�session��
	std::vector<int64_t> outputDims = ctx.get()->session->mOutputDims[0];	// ���onnx�е�һ�����ά�ȣ�session��
	auto input_name = ctx.get()->session->mInputTensorNames[0];				// ���onnx�е�һ����������
	auto output_name = ctx.get()->session->mOutputTensorNames[0];			// ���onnx�е�һ���������
	// ���onnx�е�һ���������
	// 3.1 �������ά��
	int session_batch{ 0 }, session_channels{ 0 }, session_height{ 0 }, session_width{ 0 };
	session_batch = inputDims[0];	// ���batchsize��session��
	session_channels = inputDims[1];// ���channels��session��
	session_height = inputDims[2];// ���height��session��
	session_width = inputDims[3];// ���width��session��
	// 3.2 ���input��output����
	std::vector<const char*> input_node_names;
	std::vector<const char*> output_node_names;
	output_node_names.push_back(output_name.c_str());	// ����������
	input_node_names.push_back(input_name.c_str());	// �����������
	// 3.3 onnx��batchsize�������������ݵ�batchsize�Ա�
	std::vector<cv::Mat> imgs;	// ���resize���ͼƬ�����ж�ʱ���洢��imgs��
	if (cvImages.size() != session_batch) {// batchsize�ж�
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "seg", "����ͼƬ�����batchsize��ONNX batchsize��ͬ ");
		return FF_ERROR_INPUT;
	}
	// 3.4 onnx��c,h,w�������ͼƬ�Ա�
	for (int i = 0; i < cvImages.size(); i++) {// c,h,w�ж�
		cv::Mat cv_img = cvImages[i].clone();
		if (cv_img.channels() != session_channels) {// ͨ�����ж�
			ortLog.W(__FILE__, __LINE__, YLog::INFO, "seg", "����ͼƬ��ͨ������session���� ");
			return FF_ERROR_INPUT;
		}
		if (session_height != cv_img.cols || session_width != cv_img.rows) {// h��w�ж�
			ortLog.W(__FILE__, __LINE__, YLog::INFO, "seg", "�����ͼƬ�ߴ���session�����,�Զ�resize");
			//cv::resize(cv_img, cv_img, cv::Size(session_height, session_width), 0.5, 0.5, cv::INTER_AREA); // ʹ�ô˷�����onnx��python��ʱ���Ӧ
			cv::resize(cv_img, cv_img, cv::Size(session_height, session_width), cv::INTER_LINEAR); // ʹ�ô˷�����onnx��python��ʱ���Ӧ
		}
		imgs.push_back(cv_img);
	}
	// 3.5 �ж�ͼƬ�Ƿ�Ϊ��
	if (imgs.empty()) {
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "Anomaly", "No images, please check");
		return FF_ERROR_INPUT;
	}

	// 4. Ԥ����ͼƬ
	size_t input_tensor_size = session_batch * session_channels * session_height * session_width;  // ���ͼƬ�ߴ�
	std::vector<float> input_tensor_values(input_tensor_size);	// �������ͼƬ
	auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);// create input tensor object from data values
	normalization(imgs, input_tensor_values, ctx, memory_info);// ��һ��ͼƬ�� ������洢��input_tensor_values
	Ort::Value input_tensor = Ort::Value::CreateTensor<float>(// ͨ��input_tensor_values����input_tensor
		memory_info,// ort�ڴ�����
		input_tensor_values.data(),// ��������
		input_tensor_size,// ���ݴ�С
		ctx.get()->session->mInputDims[0].data(),// onnx�����shape
		ctx.get()->session.get()->m_Session.GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape().size());
	assert(input_tensor.IsTensor());	// �ж�input_tensor�Ƿ���Tensor
	// 5.ort_inputs����
	std::vector<Ort::Value> ort_inputs;
	ort_inputs.push_back(std::move(input_tensor));
	// 6. infer�������
	auto output_tensors = ctx.get()->session.get()->m_Session.Run(
		Ort::RunOptions{ nullptr },
		input_node_names.data(),
		ort_inputs.data(),
		ort_inputs.size(),
		output_node_names.data(),
		ctx.get()->session.get()->mOutputTensorNames.size());
	assert(output_tensors.size() == 1 && output_tensors.front().IsTensor());

	// 7. ����
	// Get pointer to output tensor float values
	float* floatarr = output_tensors.front().GetTensorMutableData<float>();
	segPost(floatarr, outputs, outputDims[0], outputDims[1], outputDims[2]);
	return FF_OK;
}


// \! Detection
int ORTCORE::detect(
	std::shared_ptr<ORTCore_ctx> ctx, 
	const std::vector<cv::Mat> &cvImages,
	std::vector<std::vector<BBox>>& outputs
)
{
	return FF_OK;
}


// \! ���input������ά��
int ORTCORE::getInputDimsK(
	std::shared_ptr<ORTCore_ctx> ctx, 
	int & nBatch, 
	int & nChannels, 
	int & nHeight,
	int & nWidth
)
{
	// ��־�ļ�
	YLog ortLog(YLog::INFO, ctx.get()->params.log_path, YLog::ADD);

	if (ctx == nullptr) {
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "ORTCORE", "Init failed, can't call getDims");
		return FF_ERROR_PNULL;
	}
	nBatch = ctx->session->mInputDims[0][0];
	nChannels = ctx->session->mInputDims[0][1];
	nHeight = ctx->session->mInputDims[0][2];
	nWidth = ctx->session->mInputDims[0][3];
	return FF_OK;
}

// \! ���output�����ά��
int ORTCORE::getOutputDimsK(
	std::shared_ptr<ORTCore_ctx> ctx,
	int & nBatch,
	int & nHeight,
	int & nWidth
)
{
	// ��־�ļ�
	YLog ortLog(YLog::INFO, ctx.get()->params.log_path, YLog::ADD);

	if (ctx == nullptr) {
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "ORTCORE", "Init failed, can't call getDims");
		return FF_ERROR_PNULL;
	}
	nBatch = ctx->session->mOutputDims[0][0];
	nHeight = ctx->session->mOutputDims[0][1];
	nWidth = ctx->session->mOutputDims[0][2];
	return FF_OK;
}

// \! ���output�����ά��
int ORTCORE::getOutputDimsK(
	std::shared_ptr<ORTCore_ctx> ctx,
	int & nBatch,
	int & numClass
)
{
	// ��־�ļ�
	YLog ortLog(YLog::INFO, ctx.get()->params.log_path, YLog::ADD);

	if (ctx == nullptr) {
		ortLog.W(__FILE__, __LINE__, YLog::INFO, "ORTCORE", "Init failed, can't call getDims");
		return FF_ERROR_PNULL;
	}
	nBatch = ctx->session->mOutputDims[0][0];
	numClass = ctx->session->mOutputDims[0][1];
	return FF_OK;
}
