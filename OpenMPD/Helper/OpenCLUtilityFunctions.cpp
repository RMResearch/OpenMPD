#include <Helper/OpenCLUtilityFunctions.h>
#include <stdlib.h>
#include <stdio.h>

bool OpenCLUtilityFunctions::createProgramFromFile(const char* programFile, cl_context context, cl_device_id device, cl_program* out_program, char* compilerOptions){
   char *program_buffer, *program_log;	
   size_t program_size, log_size;
   int err;
   program_buffer = OpenCLUtilityFunctions::read_file(programFile, &program_size);
   cl_program program = clCreateProgramWithSource(context, 1, 
      (const char**)&program_buffer, &program_size, &err);
   if(err < 0) {
      perror("Couldn't create the program");
      return false;
   }

   free(program_buffer);
   /* Build program */
   err = clBuildProgram(program, 1, &device, compilerOptions, NULL, NULL);
   //err = clBuildProgram(program, 1, &device, "-cl-mad-enable -cl-finite-math-only -cl-fast-relaxed-math -cl-std=CL2.0", NULL, NULL);
   //err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
   if(err < 0) {
      /* Find size of log and print to std output */
      clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 
            0, NULL, &log_size);
      program_log = (char*) malloc(log_size + 1);
      program_log[log_size] = '\0';
      clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 
            log_size + 1, program_log, NULL);
      printf("%s\n", program_log);
      free(program_log);
      return false;
   }
   *out_program=program;
   return true;

}

/*bool OpenCLUtilityFunctions::read_image_data(const char* filename, png_bytep* data, size_t* w, size_t* h) {

   unsigned int i;
   png_structp png_ptr;
   png_infop info_ptr;

   // Open input file 
   FILE *png_input;
   if((png_input = fopen(filename, "rb")) == NULL) {
      perror("Can't read input image file");
      return false;
   }

   // Read image data
   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   info_ptr = png_create_info_struct(png_ptr);
   png_init_io(png_ptr, png_input);
   png_read_info(png_ptr, info_ptr);

   *w = png_get_image_width(png_ptr, info_ptr);
   *h = png_get_image_height(png_ptr, info_ptr);

   // Allocate memory and read image data 
   *data = (png_bytep)(malloc(*h * png_get_rowbytes(png_ptr, info_ptr)));
   for(i=0; i<*h; i++) {
      png_read_row(png_ptr, *data + i * png_get_rowbytes(png_ptr, info_ptr), NULL);
   }

   // Close input file
   png_read_end(png_ptr, info_ptr);
   png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
   fclose(png_input);
	return true;
}
*/
/* Read a character buffer from a file */
char* OpenCLUtilityFunctions::read_file(const char* filename, size_t* size) {

   FILE *handle;
   char *buffer;

   /* Read program file and place content into buffer */
   fopen_s(&handle,filename, "rb");

   if(handle == NULL) {
      perror("Couldn't find the file");
      return false;
   }
   fseek(handle, 0, SEEK_END);
   *size = (size_t)ftell(handle);
   rewind(handle);
   buffer = (char*)malloc(*size+1);
   buffer[*size] = '\0';
   fread(buffer, sizeof(char), *size, handle);
   fclose(handle);

   return buffer;
}


