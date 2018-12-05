################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
src/%.obj: ../src/%.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --include_path="C:/Users/Marcus/workspace_v8/CE4000_Team_Gamma" --include_path="C:/ti/simplelink_cc32xx_sdk_2_30_00_05/source" --include_path="C:/ti/simplelink_cc32xx_sdk_2_30_00_05/kernel/nortos" --include_path="C:/ti/simplelink_cc32xx_sdk_2_30_00_05/kernel/nortos/posix" --include_path="C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS/include" --include_path="C:/Users/Marcus/workspace_v8/CE4000_Team_Gamma/inc" --include_path="C:/Users/Marcus/workspace_v8/CE4000_Team_Gamma/inc" --define=NORTOS_SUPPORT -g --c99 --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --gen_func_subsections=on --preproc_with_compile --preproc_dependency="src/$(basename $(<F)).d_raw" --obj_directory="src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


