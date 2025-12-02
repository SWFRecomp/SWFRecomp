#include <recomp.h>
#include <constants.h>
#include <draws.h>

#include <script_decls.h>

int main()
{
	SWFAppContext app_context;
	
	app_context.frame_funcs = frame_funcs;
	
	app_context.width = FRAME_WIDTH;
	app_context.height = FRAME_HEIGHT;
	
	app_context.stage_to_ndc = stage_to_ndc;
	
	app_context.bitmap_count = BITMAP_COUNT;
	app_context.bitmap_highest_w = BITMAP_HIGHEST_W;
	app_context.bitmap_highest_h = BITMAP_HIGHEST_H;
	
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
	app_context.bitmap_data = (char*) bitmap_data;
	app_context.bitmap_data_size = sizeof(bitmap_data);
	app_context.glyph_data = (u32*) glyph_data;
	app_context.glyph_data_size = sizeof(glyph_data);
	app_context.text_data = text_data;
	app_context.text_data_size = sizeof(text_data);
	app_context.cxform_data = (char*) cxform_data;
	app_context.cxform_data_size = sizeof(cxform_data);
	
	app_context.max_string_id = MAX_STRING_ID;
	
	swfStart(&app_context);
}