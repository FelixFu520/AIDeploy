#include "trtCore.h"
#include "engine.h"
#include "Utils.h"

// \! ��ʼ��
// \@param params     ��ʼ������
// \@param nErrnoFlag ��ʼ�������룬�����params.h
std::shared_ptr<TRTCore_ctx> TRTCORE::init(const Params& params, int& nErrnoFlag) 
{
	LOG_F(INFO, "Init Start ......");
	try {
		// 1. �жϴ˵���GPU�Ƿ���ݴ����
		if (!IsCompatible(params.gpuId)) {
			nErrnoFlag = LY_WRONG_CUDA;
			LOG_F(INFO, "GPU Compatible Error");
			return nullptr;
		}
		std::string log_ = std::string("Using GPU No. ") + std::to_string(params.gpuId);
		LOG_F(INFO, log_.c_str());

		// 2. build Engine. ��������
		std::shared_ptr<TRTEngine> engine_ptr(new TRTEngine(params, nErrnoFlag));
		if (nErrnoFlag != LY_OK) {
			LOG_F(INFO, "Can't build or load engine file");
			return nullptr;
		}
		
		// 3.generate Contexts pools�� ͨ�������һЩ���ò��������ִ�������ģ��̳߳�
		ContextPool<ExecContext> pool;
		for (int i = 0; i < params.maxThread; ++i)
		{
			std::unique_ptr<ExecContext> context(new ExecContext(params.gpuId, engine_ptr->Get()));
			pool.Push(std::move(context));
		}
		if (pool.Size() == 0) {
			nErrnoFlag = LY_WRONG_CUDA;
			LOG_F(INFO, "No suitable CUDA device");
			return nullptr;
		}

		// 4.����ִ��������
		std::shared_ptr<TRTCore_ctx> ctx(new TRTCore_ctx{ params,  engine_ptr, std::move(pool) });
		LOG_F(INFO, "Init Successfully !!!!");

		return ctx;
	}
	catch (const std::invalid_argument& ex) {
		LOG_F(INFO, "Init failed !!!!");
		nErrnoFlag = LY_UNKNOW_ERROR;
		return nullptr;
	}
}

// \! ����
// \@param ctx:ִ��������
// \@param vInCoreImages:����ͼ���б�Mat��ʽ
// \@param vvOutClsRes:��������ClassifyResult��ʽ
int TRTCORE::classify(std::shared_ptr<TRTCore_ctx> ctx,const std::vector<cv::Mat> &cvImages,std::vector<std::vector<ClassifyResult>>& outputs)
{
	// 1.ctx�Ƿ��ʼ���ɹ�
	if (ctx == nullptr) {
		LOG_F(INFO, "Init Failed��Can't call classify function !");
		return LY_UNKNOW_ERROR;
	}

	// 2. �ж�NetType�Ƿ���ȷ
	if (ctx.get()->params.netType != LUSTER_CLS)
	{
		LOG_F(INFO, "Illegal calls��please check your NetWorkType");
		return LY_WRONG_CALL;
	}

	// 3. Engine��Ϣ��������Ϣ�Ա�
	int engine_batch, engine_channels, engine_height, engine_width;
	this->getInputDims(ctx, engine_batch, engine_channels, engine_height, engine_width);// �������ά����Ϣ
	int engine_output_batch, engine_output_numClass;
	this->getOutputDims2(ctx, engine_output_batch, engine_output_numClass);//������ά����Ϣ
	auto engine_input_name = this->getInputNames(ctx);  //�����������
	auto engine_output_name = this->getOutputNames(ctx);//����������
	// 3.1 batchsize�ж�
	if (cvImages.size() > engine_batch) {
		LOG_F(INFO, "����ͼƬ�����batchsize����EngineԤ��ֵ ");
		return LY_WRONG_IMG;
	}
	// 3.2 c,h,w�ж�
	std::vector<cv::Mat> imgs;
	for (int i = 0; i < cvImages.size(); i++) {
		cv::Mat cv_img = cvImages[i].clone();
		if (cv_img.channels() != engine_channels) {
			LOG_F(INFO, "����ͼƬ��ͨ������Engine���� ");
			return LY_WRONG_IMG;
		}
		if (engine_height != cv_img.cols || engine_width != cv_img.rows) {
			LOG_F(WARNING, "�����ͼƬ�ߴ���Engine�����,�Զ�resize");
			cv::resize(cv_img, cv_img, cv::Size(engine_height, engine_width), 0, 0, cv::INTER_LINEAR);
		}
		imgs.push_back(cv_img);
	}
	if (imgs.empty()) {
		LOG_F(INFO, "No images, please check");
		return LY_WRONG_IMG;
	}

	// 4.Ԥ����
	samplesCommon::BufferManager buffers(ctx.get()->engine->Get());// �����Դ棨����������
	if (LY_OK != normalization(buffers, imgs, ctx.get()->params, engine_input_name))
	{
		LOG_F(INFO, "CPU2GPU �ڴ濽��ʧ��");
		return LY_UNKNOW_ERROR;
	}
	buffers.copyInputToDevice();
	
	// 5.ִ���������
	// mContext->executeV2 �������Ŀ������ĵ�һ����룬ģ�ͼ�������һ����
	ScopedContext<ExecContext> context(ctx->pool);
	auto ctx_ = context->getContext();
	if (!ctx_->executeV2(buffers.getDeviceBindings().data()))
	{
		LOG_F(INFO, "ִ������ʧ��");
		return LY_UNKNOW_ERROR;
	}

	// 6. ����
	buffers.copyOutputToHost();
	if (LY_OK != clsPost(buffers, outputs, engine_output_batch, engine_output_numClass, engine_output_name)) {
		LOG_F(INFO, "GPU2CPU �ڴ濽��ʧ��");
		return LY_UNKNOW_ERROR;
	}

	return LY_OK;
}

// \! �ָ�
// \@param ctx: ִ��������
// \@param vInCoreImages: ����ͼƬvector��cvImage
// \@param vOutCoreImages:���ͼƬvector��cvImage
// \@param verbose: ���Ϊtrue,return the probability graph, else return the class id image
int TRTCORE::segment(std::shared_ptr<TRTCore_ctx> ctx, const std::vector<cv::Mat> &cvImages, std::vector<cv::Mat>& outputs, bool verbose)
{
	// 1. ctx�Ƿ��ʼ���ɹ�
	if (ctx == nullptr) {
		LOG_F(INFO, "Init failed, can't call segment");
		return LY_UNKNOW_ERROR;
	}

	// 2.NetType �Ƿ���ȷ
	if (ctx->params.netType != LUSTER_SEG)
	{
		LOG_F(INFO, "Illegal calls��please check your NetWorkType");
		return LY_WRONG_CALL;
	}

	// 3. Engine��Ϣ��������Ϣ�Ա�
	int engine_batch, engine_channels, engine_height, engine_width;
	this->getInputDims(ctx, engine_batch, engine_channels, engine_height, engine_width);// �������ά����Ϣ
	int engine_output_batch, engine_output_height, engine_output_width;
	this->getOutputDims(ctx, engine_output_batch, engine_output_height, engine_output_width);//������ά����Ϣ
	auto engine_input_name = this->getInputNames(ctx);  //�����������
	auto engine_output_name = this->getOutputNames(ctx);//����������
	// 3.1 batchsize�ж�
	if (cvImages.size() > engine_batch) {
		LOG_F(INFO, "����ͼƬ�����batchsize����EngineԤ��ֵ ");
		return LY_WRONG_IMG;
	}
	// 3.2 c,h,w�ж�
	std::vector<cv::Mat> imgs;//������յ�imgs
	for (int i = 0; i < cvImages.size(); i++) {
		cv::Mat cv_img = cvImages[i].clone();
		if (cv_img.channels() != engine_channels) {
			LOG_F(INFO, "����ͼƬ��ͨ������Engine���� ");
			return LY_WRONG_IMG;
		}
		if (engine_height != cv_img.cols || engine_width != cv_img.rows) {
			LOG_F(WARNING, "�����ͼƬ�ߴ���Engine�����,�Զ�resize");
			cv::resize(cv_img, cv_img, cv::Size(engine_height, engine_width), cv::INTER_LINEAR); // ʹ�ô˷�����onnx��python��ʱ���Ӧ
		}
		imgs.push_back(cv_img);
	}
	if (imgs.empty()) {
		LOG_F(INFO, "No images, please check");
		return LY_WRONG_IMG;
	}

	// 4. Ԥ����ͼƬ
	samplesCommon::BufferManager buffers(ctx.get()->engine->Get());// �����Դ棨����������
	// Ԥ����
	if (LY_OK != normalization(buffers, imgs, ctx.get()->params, engine_input_name))
	{
		LOG_F(INFO, "CPU2GPU �ڴ濽��ʧ��");
		return LY_UNKNOW_ERROR;
	}
	buffers.copyInputToDevice();

	// 5. ִ���������
	// mContext->executeV2 �������Ŀ������ĵ�һ����룬ģ�ͼ�������һ����
	ScopedContext<ExecContext> context(ctx->pool);
	auto ctx_ = context->getContext();
	if (!ctx_->executeV2(buffers.getDeviceBindings().data()))
	{
		LOG_F(INFO, "ִ������ʧ��");
		return LY_UNKNOW_ERROR;
	}

	// 6. ����
	buffers.copyOutputToHost();
	int segPostFlag = segPost(
		buffers,
		outputs,
		engine_output_batch,
		engine_output_height,
		engine_output_width,
		engine_output_name);
	if (LY_OK != segPostFlag) {
		LOG_F(INFO, "GPU2CPU �ڴ濽��ʧ��");
		return LY_UNKNOW_ERROR;
	}

	return LY_OK;
}

// \! Ŀ����
int TRTCORE::detect(
	std::shared_ptr<TRTCore_ctx> ctx, 
	const std::vector<cv::Mat> &coreImages, 
	std::vector<std::vector<BBox>>& outputs
)
{
	return LY_UNKNOW_ERROR;
}

// \! �쳣���
// \@param ctx:ִ��������
// \@param cvImages:����ͼƬ�б�Mat
// \@param outputs:���ͼƬ���飬Mat
int TRTCORE::anomaly(std::shared_ptr<TRTCore_ctx> ctx,const std::vector<cv::Mat> &cvImages,std::vector<cv::Mat>& outputs)
{
	// 1.ctx�Ƿ��ʼ���ɹ�
	if (ctx == nullptr) {
		LOG_F(INFO, "Init failed, can't call anomaly.");
		return LY_UNKNOW_ERROR;
	}

	// 2. NetType �Ƿ���ȷ
	if (ctx->params.netType != LUSTER_ANOMALY)
	{
		LOG_F(INFO, "Illegal calls��please check your NetWorkType");
		return LY_WRONG_CALL;
	}

	// 3.Engine��Ϣ��������Ϣ�Ա�
	int engine_batch, engine_channels, engine_height, engine_width;
	this->getInputDims(ctx, engine_batch, engine_channels, engine_height, engine_width);// �������ά����Ϣ
	int engine_output_batch, engine_output_height, engine_output_width;
	this->getOutputDims(ctx, engine_output_batch, engine_output_height, engine_output_width);//������ά����Ϣ
	auto engine_input_name = this->getInputNames(ctx);  //�����������
	auto engine_output_name = this->getOutputNames(ctx);//����������
	// 3.1 batchsize�ж�
	if (cvImages.size() > engine_batch) {
		LOG_F(INFO, "����ͼƬ�����batchsize����EngineԤ��ֵ ");
		return LY_WRONG_IMG;
	}
	// 3.2 c,h,w�ж�
	std::vector<cv::Mat> imgs;//������յ�imgs
	for (int i = 0; i < cvImages.size(); i++) {
		cv::Mat cv_img = cvImages[i].clone();
		if (cv_img.channels() != engine_channels) {
			LOG_F(ERROR, "����ͼƬ��ͨ������Engine���� ");
			return LY_WRONG_IMG;
		}
		if (engine_height != cv_img.cols || engine_width != cv_img.rows) {
			LOG_F(WARNING, "�����ͼƬ�ߴ���Engine�����,�Զ�resize");
			cv::resize(cv_img, cv_img, cv::Size(engine_height, engine_width), cv::INTER_LINEAR); // ʹ�ô˷�����onnx��python��ʱ���Ӧ
		}
		imgs.push_back(cv_img);
	}
	if (imgs.empty()) {
		LOG_F(INFO, "No images, please check");
		return LY_WRONG_IMG;
	}

	// 4. Ԥ����ͼƬ
	samplesCommon::BufferManager buffers(ctx.get()->engine->Get());// �����Դ棨����������
	// Ԥ����
	if (LY_OK != normalization(buffers, imgs, ctx.get()->params, engine_input_name))
	{
		LOG_F(INFO, "CPU2GPU �ڴ濽��ʧ��");
		return LY_UNKNOW_ERROR;
	}
	buffers.copyInputToDevice();
	
	// 5. ִ���������
	// mContext->executeV2 �������Ŀ������ĵ�һ����룬ģ�ͼ�������һ����
	ScopedContext<ExecContext> context(ctx->pool);
	auto ctx_ = context->getContext();
	if (!ctx_->executeV2(buffers.getDeviceBindings().data()))
	{
		LOG_F(INFO, "ִ������ʧ��");
		return LY_UNKNOW_ERROR;
	}
	
	// 6. ����
	buffers.copyOutputToHost();
	int anomalyPostFlag = anomalyPost(
		buffers,
		outputs, 
		engine_output_batch, 
		engine_output_height, 
		engine_output_width,
		engine_output_name);
	if (LY_OK != anomalyPostFlag) {
		LOG_F(INFO, "GPU2CPU �ڴ濽��ʧ��");
		return LY_UNKNOW_ERROR;
	}

	return LY_OK;

}

// \! ��ȡ�Կ�����
// \@param ctx:ִ��������
// \@param number:gpu����
int TRTCORE::getNumberGPU(std::shared_ptr<TRTCore_ctx> ctx,int& number)
{
	cudaError_t st = cudaGetDeviceCount(&number);
	if (st != cudaSuccess) {
		LOG_F(INFO, "Could not list CUDA devices");
		return 0;
	}
	return LY_OK;
}

// \! ��ȡ����ά��
// \@param ctx:ִ��������
// \@param nBatch:batchsize
// \@param nChannels:channels
// \@param nHeight:height
// \@param nWidth:width
// \@param index:��index�����룬����onnx�ж�����룬��ͨ��index��ָ��
int TRTCORE::getInputDims(std::shared_ptr<TRTCore_ctx> ctx,int & nBatch,int & nChannels,int & nHeight,int & nWidth,int index)
{
	if (ctx == nullptr) {
		LOG_F(INFO, "init failed, can't call getDims");
		return LY_UNKNOW_ERROR;
	}
	auto inputDims = ctx->engine->mInputDims[index];
	nBatch = inputDims.d[0];
	nChannels = inputDims.d[1];
	nHeight = inputDims.d[2];
	nWidth = inputDims.d[3];
	return LY_OK;
}

// \! ��ȡ���ά��
// \@param ctx��ִ��������
// \@param nBatch:batchsize
// \@param nHeight:Height
// \@param nWidth:Width
// \@param index:��index�����������onnx�ж���������ͨ��index��ָ��
int TRTCORE::getOutputDims(std::shared_ptr<TRTCore_ctx> ctx,int & nBatch,int & nHeight,int & nWidth,int index)
{
	if (ctx == nullptr) {
		LOG_F(INFO, "init failed, can't call getDims");
		return LY_UNKNOW_ERROR;
	}
	auto outputDims = ctx->engine->mOutputDims[index];
	nBatch = outputDims.d[0];
	nHeight = outputDims.d[1];
	nWidth = outputDims.d[2];
	return LY_OK;
}
// \! ��ȡ���ά��
// \@param ctx��ִ��������
// \@param nBatch:batchsize
// \@param nNumClass:NumClass ���������Է���
// \@param index:��index�����������onnx�ж���������ͨ��index��ָ��
int TRTCORE::getOutputDims2(std::shared_ptr<TRTCore_ctx> ctx,int & nBatch,int & nNumClass,int index)
{
	if (ctx == nullptr) {
		LOG_F(INFO, "init failed, can't call getDims");
		return LY_UNKNOW_ERROR;
	}
	auto outputDims = ctx->engine->mOutputDims[index];
	nBatch = outputDims.d[0];
	nNumClass = outputDims.d[1];
	return LY_OK;
}

// \! ��ȡ��������
// \@param ctx��ִ��������
// \@param index:��index�����������onnx�ж���������ͨ��index��ָ��
std::string TRTCORE::getInputNames(std::shared_ptr<TRTCore_ctx> ctx,int index)
{
	auto engine_input_name = ctx.get()->engine->mInputTensorNames[index];
	return engine_input_name;
}

// \! ��ȡ�������
// \@param ctx��ִ��������
// \@param index:��index�����������onnx�ж���������ͨ��index��ָ��
std::string TRTCORE::getOutputNames(std::shared_ptr<TRTCore_ctx> ctx,int index)
{
	auto engine_output_name = ctx.get()->engine->mOutputTensorNames[index];
	return engine_output_name;
}