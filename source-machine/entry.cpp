#include "processor.h"
#include "controller.h"
#include "ids.h"
#include "../source/version.h"

#include "public.sdk/source/main/pluginfactory.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

#define stringPluginName "125A Mechamorph Machine"

BEGIN_FACTORY_DEF(stringCompanyName, stringCompanyWeb, stringCompanyEmail)

DEF_CLASS2(INLINE_UID_FROM_FUID(MechamorphMachine::ProcessorUID),
           PClassInfo::kManyInstances,
           kVstAudioEffectClass,
           stringPluginName,
           Vst::kDistributable,
           "Instrument|Sampler",
           FULL_VERSION_STR,
           kVstVersionString,
           MechamorphMachine::Processor::createInstance)

DEF_CLASS2(INLINE_UID_FROM_FUID(MechamorphMachine::ControllerUID),
           PClassInfo::kManyInstances,
           kVstComponentControllerClass,
           stringPluginName " Controller",
           0,
           "",
           FULL_VERSION_STR,
           kVstVersionString,
           MechamorphMachine::Controller::createInstance)

END_FACTORY
