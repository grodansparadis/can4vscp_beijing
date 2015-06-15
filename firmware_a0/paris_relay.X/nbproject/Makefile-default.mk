#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-default.mk)" "nbproject/Makefile-local-default.mk"
include nbproject/Makefile-local-default.mk
endif

# Environment
MKDIR=gnumkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=default
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=cof
DEBUGGABLE_SUFFIX=cof
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/paris_relay.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=cof
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/paris_relay.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/1472/main.o ${OBJECTDIR}/_ext/1806190793/vscp_firmware.o ${OBJECTDIR}/_ext/1420683758/eeprom.o ${OBJECTDIR}/_ext/1472/ECAN.o
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/1472/main.o.d ${OBJECTDIR}/_ext/1806190793/vscp_firmware.o.d ${OBJECTDIR}/_ext/1420683758/eeprom.o.d ${OBJECTDIR}/_ext/1472/ECAN.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/1472/main.o ${OBJECTDIR}/_ext/1806190793/vscp_firmware.o ${OBJECTDIR}/_ext/1420683758/eeprom.o ${OBJECTDIR}/_ext/1472/ECAN.o


CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

# The following macros may be used in the pre and post step lines
Device=PIC18F2580
ProjectDir="C:\Users\akhe\Documents\development\hardware_zeus\can4vscp\paris_relaydriver\firmware\paris_relay.X"
ConfName=default
ImagePath="dist\default\${IMAGE_TYPE}\paris_relay.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}"
ImageDir="dist\default\${IMAGE_TYPE}"
ImageName="paris_relay.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}"

.build-conf:  .pre ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-default.mk dist/${CND_CONF}/${IMAGE_TYPE}/paris_relay.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
	@echo "--------------------------------------"
	@echo "User defined post-build step: []"
	@
	@echo "--------------------------------------"

MP_PROCESSOR_OPTION=18F2580
MP_PROCESSOR_OPTION_LD=18f2580
MP_LINKER_DEBUG_OPTION= -u_DEBUGCODESTART=0x7dc0 -u_DEBUGCODELEN=0x240 -u_DEBUGDATASTART=0x5f4 -u_DEBUGDATALEN=0xb
# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/1472/main.o: ../main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1472 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o.d 
	${MP_CC} $(MP_EXTRA_CC_PRE) -D__DEBUG -D__MPLAB_DEBUGGER_REAL_ICE=1 -p$(MP_PROCESSOR_OPTION) -k -DDEBUG -I".." -I"C:/Program Files (x86)/Microchip/mplabc18/v3.40/h" -I"C:/Users/akhe/Documents/development/m2m/firmware/pic/common" -I"C:/Users/akhe/Documents/development/m2m/firmware/common" -I"C:/Users/akhe/Documents/development/m2m/src/vscp/common" -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/main.o   ../main.c 
	@${DEP_GEN} -d ${OBJECTDIR}/_ext/1472/main.o 
	
${OBJECTDIR}/_ext/1806190793/vscp_firmware.o: ../../../../../m2m/firmware/common/vscp_firmware.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1806190793 
	@${RM} ${OBJECTDIR}/_ext/1806190793/vscp_firmware.o.d 
	${MP_CC} $(MP_EXTRA_CC_PRE) -D__DEBUG -D__MPLAB_DEBUGGER_REAL_ICE=1 -p$(MP_PROCESSOR_OPTION) -k -DDEBUG -I".." -I"C:/Program Files (x86)/Microchip/mplabc18/v3.40/h" -I"C:/Users/akhe/Documents/development/m2m/firmware/pic/common" -I"C:/Users/akhe/Documents/development/m2m/firmware/common" -I"C:/Users/akhe/Documents/development/m2m/src/vscp/common" -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1806190793/vscp_firmware.o   ../../../../../m2m/firmware/common/vscp_firmware.c 
	@${DEP_GEN} -d ${OBJECTDIR}/_ext/1806190793/vscp_firmware.o 
	
${OBJECTDIR}/_ext/1420683758/eeprom.o: ../../../../../m2m/firmware/pic/common/eeprom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1420683758 
	@${RM} ${OBJECTDIR}/_ext/1420683758/eeprom.o.d 
	${MP_CC} $(MP_EXTRA_CC_PRE) -D__DEBUG -D__MPLAB_DEBUGGER_REAL_ICE=1 -p$(MP_PROCESSOR_OPTION) -k -DDEBUG -I".." -I"C:/Program Files (x86)/Microchip/mplabc18/v3.40/h" -I"C:/Users/akhe/Documents/development/m2m/firmware/pic/common" -I"C:/Users/akhe/Documents/development/m2m/firmware/common" -I"C:/Users/akhe/Documents/development/m2m/src/vscp/common" -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1420683758/eeprom.o   ../../../../../m2m/firmware/pic/common/eeprom.c 
	@${DEP_GEN} -d ${OBJECTDIR}/_ext/1420683758/eeprom.o 
	
${OBJECTDIR}/_ext/1472/ECAN.o: ../ECAN.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1472 
	@${RM} ${OBJECTDIR}/_ext/1472/ECAN.o.d 
	${MP_CC} $(MP_EXTRA_CC_PRE) -D__DEBUG -D__MPLAB_DEBUGGER_REAL_ICE=1 -p$(MP_PROCESSOR_OPTION) -k -DDEBUG -I".." -I"C:/Program Files (x86)/Microchip/mplabc18/v3.40/h" -I"C:/Users/akhe/Documents/development/m2m/firmware/pic/common" -I"C:/Users/akhe/Documents/development/m2m/firmware/common" -I"C:/Users/akhe/Documents/development/m2m/src/vscp/common" -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/ECAN.o   ../ECAN.c 
	@${DEP_GEN} -d ${OBJECTDIR}/_ext/1472/ECAN.o 
	
else
${OBJECTDIR}/_ext/1472/main.o: ../main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1472 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o.d 
	${MP_CC} $(MP_EXTRA_CC_PRE) -p$(MP_PROCESSOR_OPTION) -k -DDEBUG -I".." -I"C:/Program Files (x86)/Microchip/mplabc18/v3.40/h" -I"C:/Users/akhe/Documents/development/m2m/firmware/pic/common" -I"C:/Users/akhe/Documents/development/m2m/firmware/common" -I"C:/Users/akhe/Documents/development/m2m/src/vscp/common" -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/main.o   ../main.c 
	@${DEP_GEN} -d ${OBJECTDIR}/_ext/1472/main.o 
	
${OBJECTDIR}/_ext/1806190793/vscp_firmware.o: ../../../../../m2m/firmware/common/vscp_firmware.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1806190793 
	@${RM} ${OBJECTDIR}/_ext/1806190793/vscp_firmware.o.d 
	${MP_CC} $(MP_EXTRA_CC_PRE) -p$(MP_PROCESSOR_OPTION) -k -DDEBUG -I".." -I"C:/Program Files (x86)/Microchip/mplabc18/v3.40/h" -I"C:/Users/akhe/Documents/development/m2m/firmware/pic/common" -I"C:/Users/akhe/Documents/development/m2m/firmware/common" -I"C:/Users/akhe/Documents/development/m2m/src/vscp/common" -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1806190793/vscp_firmware.o   ../../../../../m2m/firmware/common/vscp_firmware.c 
	@${DEP_GEN} -d ${OBJECTDIR}/_ext/1806190793/vscp_firmware.o 
	
${OBJECTDIR}/_ext/1420683758/eeprom.o: ../../../../../m2m/firmware/pic/common/eeprom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1420683758 
	@${RM} ${OBJECTDIR}/_ext/1420683758/eeprom.o.d 
	${MP_CC} $(MP_EXTRA_CC_PRE) -p$(MP_PROCESSOR_OPTION) -k -DDEBUG -I".." -I"C:/Program Files (x86)/Microchip/mplabc18/v3.40/h" -I"C:/Users/akhe/Documents/development/m2m/firmware/pic/common" -I"C:/Users/akhe/Documents/development/m2m/firmware/common" -I"C:/Users/akhe/Documents/development/m2m/src/vscp/common" -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1420683758/eeprom.o   ../../../../../m2m/firmware/pic/common/eeprom.c 
	@${DEP_GEN} -d ${OBJECTDIR}/_ext/1420683758/eeprom.o 
	
${OBJECTDIR}/_ext/1472/ECAN.o: ../ECAN.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1472 
	@${RM} ${OBJECTDIR}/_ext/1472/ECAN.o.d 
	${MP_CC} $(MP_EXTRA_CC_PRE) -p$(MP_PROCESSOR_OPTION) -k -DDEBUG -I".." -I"C:/Program Files (x86)/Microchip/mplabc18/v3.40/h" -I"C:/Users/akhe/Documents/development/m2m/firmware/pic/common" -I"C:/Users/akhe/Documents/development/m2m/firmware/common" -I"C:/Users/akhe/Documents/development/m2m/src/vscp/common" -Ou- -Ot- -Ob- -Op- -Or- -Od- -Opa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1472/ECAN.o   ../ECAN.c 
	@${DEP_GEN} -d ${OBJECTDIR}/_ext/1472/ECAN.o 
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/paris_relay.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_LD} $(MP_EXTRA_LD_PRE)   -p$(MP_PROCESSOR_OPTION_LD)  -w -x -u_DEBUG -m"$(BINDIR_)$(TARGETBASE).map" -l"../../../../../../../../../Program Files (x86)/Microchip/mplabc18/v3.40/lib"  -z__MPLAB_BUILD=1  -u_CRUNTIME -z__MPLAB_DEBUG=1 -z__MPLAB_DEBUGGER_REAL_ICE=1 $(MP_LINKER_DEBUG_OPTION) -l ${MP_CC_DIR}\\..\\lib  -o dist/${CND_CONF}/${IMAGE_TYPE}/paris_relay.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}   
else
dist/${CND_CONF}/${IMAGE_TYPE}/paris_relay.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_LD} $(MP_EXTRA_LD_PRE)   -p$(MP_PROCESSOR_OPTION_LD)  -w  -m"$(BINDIR_)$(TARGETBASE).map" -l"../../../../../../../../../Program Files (x86)/Microchip/mplabc18/v3.40/lib"  -z__MPLAB_BUILD=1  -u_CRUNTIME -l ${MP_CC_DIR}\\..\\lib  -o dist/${CND_CONF}/${IMAGE_TYPE}/paris_relay.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}   
endif

.pre:
	@echo "--------------------------------------"
	@echo "User defined pre-build step: []"
	@
	@echo "--------------------------------------"

# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/default
	${RM} -r dist/default

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
