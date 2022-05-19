#define TYPE_TRT_CORE_API_EXPORTS
#include "trtCore.h"
#include "engine.h"
#include "my_logger.h"

int normalization(const samplesCommon::BufferManager & buffers, std::vector<cv::Mat>& cv_images, const Params& params, const std::string inputName);
bool IsCompatible(int device);

/*
  ____    ___    __  __   __  __    ___    _   _
 / ___|  / _ \  |  \/  | |  \/  |  / _ \  | \ | |
| |     | | | | | |\/| | | |\/| | | | | | |  \| |
| |___  | |_| | | |  | | | |  | | | |_| | | |\  |
 \____|  \___/  |_|  |_| |_|  |_|  \___/  |_| \_|
*/
// init
std::shared_ptr<TRTCore_ctx> TRTCORE::init(const Params& params, int& nErrnoFlag) {
	try {
		my_logger::log(params.log_path, std::string("Current version: 2021-12-31(1.0.0.9)"));

		if (!IsCompatible(params.gpuId)) {
			nErrnoFlag = LY_WRONG_CUDA;
			my_logger::log(params.log_path, std::string("GPU Compatible Error"));
			OutputDebugString("GPU Compatible Error");
			return nullptr;
		}
		my_logger::log(params.log_path, std::string("Using No. ") + std::to_string(params.gpuId));

		// 1.build Engine. ��������
		std::shared_ptr<TRTEngine> engine_ptr(new TRTEngine(params, nErrnoFlag));
		if (nErrnoFlag != LY_OK) {
			my_logger::log(params.log_path, std::string("Can't build or load engine file"));
			OutputDebugString("Can't build or load engine file");
			return nullptr;
		}
		// 2.generate Contexts pools�� ͨ�������һЩ���ò��������ִ�������ģ��̳߳�
		ContextPool<ExecContext> pool;
		for (int i = 0; i < params.maxThread; ++i)
		{
			std::unique_ptr<ExecContext> context(new ExecContext(params.gpuId, engine_ptr->Get()));
			pool.Push(std::move(context));
		}
		if (pool.Size() == 0) {
			nErrnoFlag = LY_WRONG_CUDA;
			my_logger::log(params.log_path, std::string("No suitable CUDA device"));
			OutputDebugString("No suitable CUDA device");
			return nullptr;
		}

		std::shared_ptr<TRTCore_ctx> ctx(new TRTCore_ctx{ params,  engine_ptr, std::move(pool) });
		my_logger::log(params.log_path, std::string("Init Successfully !!!!"));
		return ctx;
	}
	catch (const std::invalid_argument& ex) {
		my_logger::log(params.log_path, std::string("Init failed !!!!"));
		my_logger::log(params.log_path, std::string(ex.what()));
		nErrnoFlag = LY_UNKNOW_ERROR;
		OutputDebugString("Init Failed !!!!");
		return nullptr;
	}
}

// get nums of gpu
int TRTCORE::getDevices()
{
	int nDevices = 0;
	cudaError_t st = cudaGetDeviceCount(&nDevices);
	if (st != cudaSuccess) {
		OutputDebugString("Could not list CUDA devices");
		return 0;
	}
	return nDevices;
}

// get batchsize, channels, h, w
int TRTCORE::getDims(std::shared_ptr<TRTCore_ctx> ctx, int & nBatch, int & nChannels, int & nHeight, int & nWidth)
{
	if (ctx == nullptr) {
		OutputDebugString("Init failed, can't call getDims");
		return LY_UNKNOW_ERROR;
	}
	nBatch = ctx->engine->mInputDims.d[0];
	nChannels = ctx->engine->mInputDims.d[1];
	nHeight = ctx->engine->mInputDims.d[2];
	nWidth = ctx->engine->mInputDims.d[3];
	return LY_OK;
}

/*
  ____   _          _      ____    ____    ___   _____  __   __
 / ___| | |        / \    / ___|  / ___|  |_ _| |  ___| \ \ / /
| |     | |       / _ \   \___ \  \___ \   | |  | |_     \ V /
| |___  | |___   / ___ \   ___) |  ___) |  | |  |  _|     | |
 \____| |_____| /_/   \_\ |____/  |____/  |___| |_|       |_|
*/
bool PairCompare(const std::pair<float, int>& lhs, const std::pair<float, int>& rhs)
{
	return lhs.first > rhs.first;
}

//! \brief ���������з�������N��÷����,�����С�������Ŀc���������СN
std::vector<int> Argmax(const std::vector<float>& v, int N)
{
	std::vector<std::pair<float, int>> pairs;
	for (size_t i = 0; i < v.size(); ++i)
		pairs.push_back(std::make_pair(v[i], i));

	std::partial_sort(pairs.begin(), pairs.begin() + N, pairs.end(), PairCompare);

	std::vector<int> result;
	for (int i = 0; i < N; ++i)
		result.push_back(pairs[i].second);
	return result;
}

int clsPostOutputONNX(const samplesCommon::BufferManager & buffers, std::vector<std::vector<ClassifyResult>>& outputs, const int batch, const int num_class, const Params& params, std::string output_name)
{
	float* output_buffer = static_cast<float*>(buffers.getHostBuffer(output_name));

	// Top K
	int N = 3;
	auto K = N > num_class ? num_class : N;

	for (int b = 0; b < batch; b++) {
		float sum{ 0.0f };
		for (int i = 0; i < num_class; i++) {
			output_buffer[b * num_class + i] = exp(output_buffer[b * num_class + i]);
			sum += output_buffer[b * num_class + i];
		}

		// output���һ��ͼƬ�������������Ŷ�
		std::vector<float> output;
		for (int j = 0; j < num_class; j++) {
			output.push_back(output_buffer[b * num_class + j] / sum);
		}

		// output topk��index ����maxN��
		std::vector<int> maxN = Argmax(output, K);
		std::vector<ClassifyResult> classifyResults;

		for (int i = 0; i < K; ++i)
		{
			int idx = maxN[i];
			classifyResults.push_back(std::make_pair(idx, output[idx]));
		}

		outputs.push_back(classifyResults);
	}
	return LY_OK;
}

int TRTCORE::classify(std::shared_ptr<TRTCore_ctx> ctx, const std::vector<cv::Mat> &cvImages, std::vector<std::vector<ClassifyResult>>& outputs)
{
	// ctx�Ƿ��ʼ���ɹ�
	if (ctx == nullptr) {
		OutputDebugString("Init Failed��Can't call classify function !");
		return LY_UNKNOW_ERROR;
	}
	
	// �ж�NetType�Ƿ���ȷ
	if (ctx.get()->params.netType != LUSTER_CLS)
	{
		my_logger::log(ctx.get()->params.log_path, std::string("Illegal calls��please check your NetWorkType"));
		OutputDebugString("Illegal calls��please check your NetWorkType");
		return LY_WRONG_CALL;
	}

	// Engine��Ϣ��������Ϣ�Ա�
	nvinfer1::Dims inputDims = ctx.get()->engine->mInputDims;
	nvinfer1::Dims outputDims = ctx.get()->engine->mOutputDims;
	int engine_batch{ 0 }, engine_channels{ 0 }, engine_height{ 0 }, engine_width{ 0 };
	std::vector<cv::Mat> imgs;
	// �ж����������onnx�����Ƿ�һ��
	auto input_name = ctx.get()->engine->Get()->getBindingName(0);  //.get().getBindingName(0);
	auto output_name = ctx.get()->engine->Get()->getBindingName(1);
	// ���engine��Ϣ
	engine_batch = inputDims.d[0];
	engine_channels = inputDims.d[1];
	engine_height = inputDims.d[2];
	engine_width = inputDims.d[3];
	// 1. batchsize�ж�
	if (cvImages.size() > engine_batch) {
		my_logger::log(ctx.get()->params.log_path, std::string("����ͼƬ�����batchsize����EngineԤ��ֵ "), my_logger::Level::Error);
		OutputDebugString("����ͼƬ�����batchsize����EngineԤ��ֵ ");
		return LY_WRONG_IMG;
	}
	// 2. c,h,w�ж�
	for (int i = 0; i < cvImages.size(); i++) {
		cv::Mat cv_img = cvImages[i].clone();
		if (cv_img.channels() != engine_channels) {
			my_logger::log(ctx.get()->params.log_path, std::string("����ͼƬ��ͨ������Engine���� "), my_logger::Level::Error);
			OutputDebugString("����ͼƬ��ͨ������Engine���� ");
			return LY_WRONG_IMG;
		}
		if (engine_height != cv_img.cols || engine_width != cv_img.rows) {
			my_logger::log(ctx.get()->params.log_path, std::string("�����ͼƬ�ߴ���Engine�����,�Զ�resize"), my_logger::Level::Warning);
			cv::resize(cv_img, cv_img, cv::Size(engine_height, engine_width), 0, 0, cv::INTER_LINEAR);
		}

		imgs.push_back(cv_img);
	}
	if (imgs.empty()) {
		my_logger::log(ctx.get()->params.log_path, std::string("No images, please check"));
		OutputDebugString("No images, please check");
		return LY_WRONG_IMG;
	}
	

	// ONNXģ��ִ���������
	// �����Դ棨����������
	samplesCommon::BufferManager buffers(ctx.get()->engine->Get());

	// Ԥ����
	if (LY_OK != normalization(buffers, imgs, ctx.get()->params, ctx.get()->engine->Get()->getBindingName(0)))
	{
		my_logger::log(ctx.get()->params.log_path, std::string("CPU2GPU �ڴ濽��ʧ��"));
		OutputDebugString("CPU2GPU �ڴ濽��ʧ��");
		return LY_UNKNOW_ERROR;
	}

	buffers.copyInputToDevice();

	// mContext->executeV2 �������Ŀ������ĵ�һ����룬ģ�ͼ�������һ����
	ScopedContext<ExecContext> context(ctx->pool);
	auto ctx_ = context->getContext();
	if (!ctx_->executeV2(buffers.getDeviceBindings().data()))
	{
		my_logger::log(ctx.get()->params.log_path, std::string("ִ������ʧ��"));
		OutputDebugString("ִ������ʧ��");
		return LY_UNKNOW_ERROR;
	}
	buffers.copyOutputToHost();
	if (LY_OK != clsPostOutputONNX(buffers, outputs, imgs.size(), ctx.get()->engine->mOutputDims.d[1], ctx.get()->params, ctx.get()->engine->Get()->getBindingName(1))) {
		my_logger::log(ctx.get()->params.log_path, std::string("GPU2CPU �ڴ濽��ʧ��"));
		OutputDebugString("GPU2CPU �ڴ濽��ʧ��");
		return LY_UNKNOW_ERROR;
	}

	return LY_OK;
}


/*
 ____    _____    ____   __  __   _____   _   _   _____
/ ___|  | ____|  / ___| |  \/  | | ____| | \ | | |_   _|
\___ \  |  _|   | |  _  | |\/| | |  _|   |  \| |   | |
 ___) | | |___  | |_| | | |  | | | |___  | |\  |   | |
|____/  |_____|  \____| |_|  |_| |_____| |_| \_|   |_|
*/
int segPostOutput(const samplesCommon::BufferManager & buffers, std::vector<cv::Mat>& out_masks, const int numSamples,
	const Params& params, const nvinfer1::Dims outputDims, bool verbose, const std::string outputName)
{
	out_masks.clear();
	float* output = static_cast<float*>(buffers.getHostBuffer(outputName));
	int32_t num_class, height, width; 

	if (verbose) // �������ͼ
	{
		num_class = outputDims.d[1];
		height = outputDims.d[2];
		width = outputDims.d[3];
		for (int i = 0; i < numSamples; i++)
		{
			std::vector<cv::Mat> n_channels;
			//cv::Mat res = cv::Mat(height, width, CV_32FC(num_class), output + height * width * num_class * i).clone();
			cv::Mat res;
			for (int j = 0; j < num_class; j++) {
				cv::Mat tmp = cv::Mat(height, width, CV_32F, output + height * width * j + height * width * i * num_class).clone();
				n_channels.push_back(tmp);
			}
			cv::merge(n_channels, res);
			out_masks.push_back(res);
		}
	}
	else
		{
			if (outputDims.nbDims == 3)  // onnx�����������������ת���������Խ�࣬���ַ�ʽԽ����Ч������
			{
				num_class = 0;  // ������ͼ�������Ѿ�û��num_class��num_class������ֵ������
				height = outputDims.d[1];
				width = outputDims.d[2];

				for (int i = 0; i < numSamples; i++)
				{
					cv::Mat res = cv::Mat(height, width, CV_32F, output + height * width * i).clone();
					res.convertTo(res, CV_8UC1);
					out_masks.push_back(res);
				}
			}
			else  // onnx�����Ǹ���ֵ����Ҫ�Լ�����ת����Ч�ʵͣ�Ϊ�˼���֮ǰ�Ĳű����˶δ����֧�����ڽ�����֧�֡�
			{
				num_class = outputDims.d[1];
				height = outputDims.d[2];
				width = outputDims.d[3];
				if (num_class == 2)
				{  // ֻ��Ҫ��ǰ���������ָ��ʱ��������ʽ�Ĵ���Ч�ʸߡ��ɶ���ǿ
					for (int n = 0; n < numSamples; n++)
					{
						// �������h*w�ľ��󡣵�һ����bg�ĸ��ʣ��ڶ�����crack�ĸ��ʡ�����ĸ�����ӵ���1
						// ��bg����0.5Ϊ��ֵ��ֵ����С��0.5��Ϊcrack
						cv::Mat tmp = cv::Mat(height, width, CV_32F, output + height * width * (2 * n)).clone();
						cv::threshold(tmp, tmp, 0.5f, 1.f, cv::THRESH_BINARY_INV);
						tmp.convertTo(tmp, CV_8UC1);
						out_masks.push_back(tmp);
					}
				}
				else
				{	// ���������2�����
					for (int n = 0; n < numSamples; n++)
					{
						std::vector<cv::Mat> n_channels;
						//cv::Mat res = cv::Mat(height, width, CV_32FC(num_class), output + height * width * num_class * i).clone();
						cv::Mat res;
						for (int j = 0; j < num_class; j++) {
							cv::Mat tmp = cv::Mat(height, width, CV_32F, output + height * width * j + height * width * n * num_class).clone();
							n_channels.push_back(tmp);
						}
						cv::merge(n_channels, res);
						res.convertTo(res, CV_8UC(num_class));

						cv::Mat tmp = cv::Mat::zeros(height, width, CV_8U);
						for (int h = 0; h < height; ++h) {
							for (int w = 0; w < width; ++w) {
								//uchar *p = res.ptr(h, w); // prob of a point
								uchar *p = res.ptr(h, w); // prob of a point
								tmp.at<uchar>(h, w) = (uchar)std::distance(p, std::max_element(p, p + num_class));
							}
						}
						// todo Ƕ��ѭ��Ч�ʼ��͡�����
						//cv::Mat tmp = cv::Mat::zeros(height, width, CV_32F);
						//for (int row = 0; row < height; row++) {
						//	float* dataP = tmp.ptr<float>(row);
						//	for (int col = 0; col < width; col++) {
						//		std::vector<float> prop(num_class, 0.f);
						//		for (int indexC = 0; indexC < num_class; indexC++) {
						//			prop[indexC] = output[n * num_class * height * width + indexC * height * width + row * height + col];
						//		}
						//		size_t maxIndex = std::distance(prop.begin(), std::max_element(prop.begin(), prop.end()));
						//		//size_t maxIndex = argmax(prop.begin(), prop.end());
						//		dataP[col] = float(maxIndex);
						//	}
						//}
						//tmp.convertTo(tmp, CV_8UC1);

						out_masks.push_back(tmp);
					}
				}
			}
		}

	return LY_OK;
}

int TRTCORE::segment(std::shared_ptr<TRTCore_ctx> ctx, const std::vector<cv::Mat> &cvImages, std::vector<cv::Mat>& outputs, bool verbose)
{
	// ctx�Ƿ��ʼ���ɹ�
	if (ctx == nullptr) {
		OutputDebugString("Init failed, can't call segment.");
		return LY_UNKNOW_ERROR;
	}

	// NetType �Ƿ���ȷ
	if (ctx->params.netType != LUSTER_SEG)
	{
		my_logger::log(ctx->params.log_path, std::string("Illegal calls��please check your NetWorkType"));
		OutputDebugString("Illegal calls��please check your NetWorkType");
		return LY_WRONG_CALL;
	}
	
	// Engine��Ϣ��������Ϣ�Ա�
	nvinfer1::Dims inputDims = ctx.get()->engine->mInputDims;
	nvinfer1::Dims outputDims = ctx.get()->engine->mOutputDims;
	int engine_batch{ 0 }, engine_channels{ 0 }, engine_height{ 0 }, engine_width{ 0 };
	std::vector<cv::Mat> imgs;
	// �ж����������onnx�����Ƿ�һ��
	auto input_name = ctx.get()->engine->Get()->getBindingName(0);  //.get().getBindingName(0);
	auto output_name = ctx.get()->engine->Get()->getBindingName(1);
	// ���engine��Ϣ
	engine_batch = inputDims.d[0];
	engine_channels = inputDims.d[1];
	engine_height = inputDims.d[2];
	engine_width = inputDims.d[3];
	// 1. batchsize�ж�
	if (cvImages.size() > engine_batch) {
		my_logger::log(ctx.get()->params.log_path, std::string("����ͼƬ�����batchsize����EngineԤ��ֵ "), my_logger::Level::Error);
		OutputDebugString("����ͼƬ�����batchsize����EngineԤ��ֵ ");
		return LY_WRONG_IMG;
	}
	// 2. c,h,w�ж�
	for (int i = 0; i < cvImages.size(); i++) {
		cv::Mat cv_img = cvImages[i].clone();
		if (cv_img.channels() != engine_channels) {
			my_logger::log(ctx.get()->params.log_path, std::string("����ͼƬ��ͨ������Engine���� "), my_logger::Level::Error);
			OutputDebugString("����ͼƬ��ͨ������Engine���� ");
			return LY_WRONG_IMG;
		}
		if (engine_height != cv_img.cols || engine_width != cv_img.rows) {
			my_logger::log(ctx.get()->params.log_path, std::string("�����ͼƬ�ߴ���Engine�����,�Զ�resize"), my_logger::Level::Warning);
			cv::resize(cv_img, cv_img, cv::Size(engine_height, engine_width), 0, 0, cv::INTER_LINEAR);
		}

		imgs.push_back(cv_img);
	}
	if (imgs.empty()) {
		my_logger::log(ctx.get()->params.log_path, std::string("No images, please check"));
		OutputDebugString("No images, please check");
		return LY_WRONG_IMG;
	}

	
	// ONNXģ��ִ���������
	// �����Դ棨����������
	samplesCommon::BufferManager buffers(ctx.get()->engine->Get());

	// Ԥ����
	if (LY_OK != normalization(buffers, imgs, ctx.get()->params, ctx.get()->engine->Get()->getBindingName(0)))
	{
		my_logger::log(ctx.get()->params.log_path, std::string("CPU2GPU �ڴ濽��ʧ��"));
		OutputDebugString("CPU2GPU �ڴ濽��ʧ��");
		return LY_UNKNOW_ERROR;
	}

	buffers.copyInputToDevice();

	// mContext->executeV2 �������Ŀ������ĵ�һ����룬ģ�ͼ�������һ����
	ScopedContext<ExecContext> context(ctx->pool);
	auto ctx_ = context->getContext();
	if (!ctx_->executeV2(buffers.getDeviceBindings().data()))
	{
		my_logger::log(ctx.get()->params.log_path, std::string("ִ������ʧ��"));
		OutputDebugString("ִ������ʧ��");
		return LY_UNKNOW_ERROR;
	}
	buffers.copyOutputToHost();
	
	if (LY_OK != segPostOutput(buffers, outputs, imgs.size(), ctx->params, outputDims, verbose, output_name)) {
		my_logger::log(ctx.get()->params.log_path, std::string("GPU2CPU �ڴ濽��ʧ��"));
		OutputDebugString("GPU2CPU �ڴ濽��ʧ��");
		return LY_UNKNOW_ERROR;
	}

	// ��mask resize��ͼƬ�����С
	for (int i = 0; i < cvImages.size(); i++)
	{
		cv::resize(outputs[i], outputs[i], cv::Size(cvImages[i].cols, cvImages[i].rows), 0, 0, cv::INTER_LINEAR);
	}
	return LY_OK;
}


/*
 ____    _____   _____   _____    ____   _____   ___    ___    _   _
|  _ \  | ____| |_   _| | ____|  / ___| |_   _| |_ _|  / _ \  | \ | |
| | | | |  _|     | |   |  _|   | |       | |    | |  | | | | |  \| |
| |_| | | |___    | |   | |___  | |___    | |    | |  | |_| | | |\  |
|____/  |_____|   |_|   |_____|  \____|   |_|   |___|  \___/  |_| \_|
*/

int TRTCORE::detect(std::shared_ptr<TRTCore_ctx> ctx, const std::vector<cv::Mat> &coreImages, std::vector<std::vector<BBox>>& outputs)
{
	return LY_UNKNOW_ERROR;
}


/*
 _   _   _____   ___   _       ____
| | | | |_   _| |_ _| | |     / ___|
| | | |   | |    | |  | |     \___ \
| |_| |   | |    | |  | |___   ___) |
 \___/    |_|   |___| |_____| |____/
*/
int normalization(const samplesCommon::BufferManager & buffers, std::vector<cv::Mat>& cv_images, const Params& params, std::string inputName)
{
	// ����host�ڴ�
	float* hostDataBuffer = static_cast<float*>(buffers.getHostBuffer(inputName));
	int nums = cv_images.size();
	int depth = cv_images[0].channels();
	int height = cv_images[0].rows;
	int width = cv_images[0].cols;

	// ͨ����Ϊ1
	if (depth == 1) {
		for (int n = 0, index = 0; n < nums; n++)
		{
			cv_images[n].convertTo(cv_images[n], CV_32F, 1.0 / 255);
			cv_images[n] = (cv_images[n] - params.meanValue[0]) / params.stdValue[0];
			memcpy_s(hostDataBuffer + n * height * width, height * width * sizeof(float), cv_images[n].data, height * width * sizeof(float));
		}
	}
	else if (depth == 3) {
		for (int n = 0, index=0; n < nums; n++)
		{
			cv_images[n].convertTo(cv_images[n], CV_32F, 1.0 / 255);
			std::vector<cv::Mat> bgrChannels(3);
			cv::split(cv_images[n], bgrChannels);
			for (int d = 0; d < 3; d++) {
				bgrChannels[2 - d] = (bgrChannels[2 - d] - params.meanValue[d]) / params.stdValue[d];	// ��ǰͼ��ͨ����RGB��ת��BGR
				memcpy_s(hostDataBuffer + height * width * (3 * n + (2 - d)), height * width * sizeof(float), bgrChannels[2 - d].data, height * width * sizeof(float));
				//memcpy_s(hostDataBuffer + height * width * (cv_images.size() * (2 - d) + n), height * width * sizeof(float), bgrChannels[2 - d].data, height * width * sizeof(float));
			}
			// һ������һ�����ؿ����� �����ٶȱȽ��������������޸�
			//std::vector<cv::Mat> bgrChannels(3);
			//cv::split(cv_images[n], bgrChannels);
			//for (int d = 0; d < depth; d++) {
			//	for (int r = 0; r < height; r++)
			//	{
			//		//float* data = bgrChannels[2 - d].ptr<float>(r);  // opencv��ȡ��ʽ��bgr�����������ط�ȫ��rgb��
			//		unchar* data = bgrChannels[2 - d].ptr<unchar>(r);  // opencv��ȡ��ʽ��bgr�����������ط�ȫ��rgb��
			//		for (int c = 0; c < width; c++)
			//		{
			//			/*hostdatabuffer[c + r * height + d * height * width + n * depth * height * width] =
			//				(1.0 * float(data[c]) / 255.0 - params.meanvalue[d]) / params.stdValue[d];*/
			//			hostDataBuffer[index++] = (1.0 * float(data[c]) / 255.0 - params.meanValue[d]) / params.stdValue[d];
			//		}
			//	}
			//}
		}
	}
	else {
		my_logger::log(params.log_path, std::string("��֧�ֵ�ͼ������"));
		OutputDebugString("��֧�ֵ�ͼ������");
		return LY_WRONG_IMG;
	}
	return LY_OK;
}

bool IsCompatible(int device)
{
	cudaError_t st = cudaSetDevice(device);
	if (st != cudaSuccess)
		return false;

	cudaDeviceProp deviceProp;
	cudaGetDeviceProperties(&deviceProp, device);
	if (deviceProp.major < 3)
		return false;

	return true;
}