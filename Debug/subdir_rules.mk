################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=vfplib -me --include_path="C:/Users/Marcus/workspace_v8/gpiointerrupt_CC3220SF_LAUNCHXL_nortos_ccs" --include_path="C:/ti/simplelink_cc32xx_sdk_2_30_00_05/source" --include_path="C:/ti/simplelink_cc32xx_sdk_2_30_00_05/kernel/nortos" --include_path="C:/ti/simplelink_cc32xx_sdk_2_30_00_05/kernel/nortos/posix" --include_path="C:/ti/ccsv8/tools/compiler/ti-cgt-arm_18.1.4.LTS/include" --define=NORTOS_SUPPORT -g --c99 --diag_warning=225 --diag_warning=255 --diag_wrap=off --display_error_number --gen_func_subsections=on --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


