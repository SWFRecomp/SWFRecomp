#include <recomp.h>
#include <constants.h>
#include <draws.h>

int main()
{
	SWFAppContext app_context;
	app_context.frame_funcs = frame_funcs;
	app_context.width = FRAME_WIDTH;
	app_context.height = FRAME_HEIGHT;
	app_context.stage_to_ndc = stage_to_ndc;
	
	app_context.shape_data = (char*) shape_data;
	app_context.shape_data_size = sizeof(shape_data);
	app_context.transform_data = (char*) transform_data;
	app_context.transform_data_size = sizeof(transform_data);
	app_context.color_data = (char*) color_data;
	app_context.color_data_size = sizeof(color_data);
	app_context.uninv_mat_data = (char*) uninv_mat_data;
	app_context.uninv_mat_data_size = sizeof(uninv_mat_data);
	app_context.gradient_data = (char*) gradient_data;
	app_context.gradient_data_size = sizeof(gradient_data);
	
	swfStart(&app_context);
}