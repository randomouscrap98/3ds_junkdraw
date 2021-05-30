

//extern C2Di_Context __C2Di_Context;

//bool My_C2D_Init(size_t maxObjects)
//{
//
//	C2Di_Context* ctx = C2Di_GetContext();
//	if (ctx->flags & C2DiF_Active)
//		return false;
//
//	ctx->vtxBufSize = 6*maxObjects;
//	ctx->vtxBuf = (C2Di_Vertex*)linearAlloc(ctx->vtxBufSize*sizeof(C2Di_Vertex));
//	if (!ctx->vtxBuf)
//		return false;
//
//	ctx->shader = DVLB_ParseFile((u32*)render2d_shbin, render2d_shbin_size);
//	if (!ctx->shader)
//	{
//		linearFree(ctx->vtxBuf);
//		return false;
//	}
//
//	shaderProgramInit(&ctx->program);
//	shaderProgramSetVsh(&ctx->program, &ctx->shader->DVLE[0]);
//
//	AttrInfo_Init(&ctx->attrInfo);
//	AttrInfo_AddLoader(&ctx->attrInfo, 0, GPU_FLOAT,         3); // v0=position
//	AttrInfo_AddLoader(&ctx->attrInfo, 1, GPU_FLOAT,         2); // v1=texcoord
//	AttrInfo_AddLoader(&ctx->attrInfo, 2, GPU_FLOAT,         2); // v2=blend
//	AttrInfo_AddLoader(&ctx->attrInfo, 3, GPU_UNSIGNED_BYTE, 4); // v3=color
//
//	BufInfo_Init(&ctx->bufInfo);
//	BufInfo_Add(&ctx->bufInfo, ctx->vtxBuf, sizeof(C2Di_Vertex), 4, 0x3210);
//
//	// Cache these common projection matrices
//	Mtx_OrthoTilt(&s_projTop, 0.0f, 400.0f, 240.0f, 0.0f, 1.0f, -1.0f, true);
//	Mtx_OrthoTilt(&s_projBot, 0.0f, 320.0f, 240.0f, 0.0f, 1.0f, -1.0f, true);
//
//	// Get uniform locations
//	uLoc_mdlvMtx = shaderInstanceGetUniformLocation(ctx->program.vertexShader, "mdlvMtx");
//	uLoc_projMtx = shaderInstanceGetUniformLocation(ctx->program.vertexShader, "projMtx");
//
//	// Prepare proctex
//	C3D_ProcTexInit(&ctx->ptBlend, 0, 1);
//	C3D_ProcTexClamp(&ctx->ptBlend, GPU_PT_CLAMP_TO_EDGE, GPU_PT_CLAMP_TO_EDGE);
//
//   //TODO: THIS IS WHAT CHANGED!!
//	C3D_ProcTexCombiner(&ctx->ptBlend, true, GPU_PT_U, GPU_PT_U);
//
//	C3D_ProcTexFilter(&ctx->ptBlend, GPU_PT_LINEAR);
//
//	C3D_ProcTexInit(&ctx->ptCircle, 0, 1);
//	C3D_ProcTexClamp(&ctx->ptCircle, GPU_PT_MIRRORED_REPEAT, GPU_PT_MIRRORED_REPEAT);
//	C3D_ProcTexCombiner(&ctx->ptCircle, true, GPU_PT_SQRT2, GPU_PT_SQRT2);
//	C3D_ProcTexFilter(&ctx->ptCircle, GPU_PT_LINEAR);
//
//	// Prepare proctex lut
//	float data[129];
//	int i;
//	for (i = 0; i <= 128; i ++)
//		data[i] = i/128.0f;
//	ProcTexLut_FromArray(&ctx->ptBlendLut, data);
//
//	for (i = 0; i <= 128; i ++)
//		data[i] = (i >= 127) ? 0 : 1;
//	ProcTexLut_FromArray(&ctx->ptCircleLut, data);
//
//	ctx->flags = C2DiF_Active;
//	ctx->vtxBufPos = 0;
//	ctx->vtxBufLastPos = 0;
//	Mtx_Identity(&ctx->projMtx);
//	Mtx_Identity(&ctx->mdlvMtx);
//	ctx->fadeClr = 0;
//
//	C3D_FrameEndHook(C2Di_FrameEndHook, NULL);
//	return true;
//}
