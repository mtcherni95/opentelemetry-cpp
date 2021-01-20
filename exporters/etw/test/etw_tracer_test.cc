// Copyright 2020, OpenTelemetry Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifdef _WIN32

#  include <gtest/gtest.h>
#  include <map>
#  include <string>

#  include "opentelemetry/exporters/etw/etw_tracer_exporter.h"
#  include "opentelemetry/sdk/trace/simple_processor.h"

using namespace OPENTELEMETRY_NAMESPACE;

using Properties       = opentelemetry::exporter::ETW::Properties;
using PropertyValue    = opentelemetry::exporter::ETW::PropertyValue;
using PropertyValueMap = opentelemetry::exporter::ETW::PropertyValueMap;

std::string getTemporaryValue()
{
  return std::string("Value from Temporary std::string");
}

/* clang-format off */
TEST(ETWTracer, TracerCheck)
{
  // SDK customer specifies their unique ETW ProviderName. Every component or library
  // is assumed to have its own instrumentation name. Traces are routed to dedicated
  // provider. Standard hash function maps from ProviderName to ProviderGUID.
  //
  // Prominent naming examples from `logman query providers` :
  //
  // [Docker]                                 {a3693192-9ed6-46d2-a981-f8226c8363bd}
  // ...
  // Intel-Autologger-iclsClient              {B8D7E9A0-65D5-40BE-AFEA-83593FC0164E}
  // Intel-Autologger-iclsProxy                {301B773F-50F3-4C8E-83F0-53BA9590A13E}
  // Intel-Autologger-PTTEKRecertification    {F33E9E07-8792-47E8-B3FA-2C92AB32C5B3}
  // ...
  // NodeJS-ETW-provider                      {77754E9B-264B-4D8D-B981-E4135C1ECB0C}
  // ...
  // OpenSSH                                  {C4B57D35-0636-4BC3-A262-370F249F9802}
  // ...
  // Windows Connect Now                      {C100BECE-D33A-4A4B-BF23-BBEF4663D017}
  // Windows Defender Firewall API            {28C9F48F-D244-45A8-842F-DC9FBC9B6E92}
  // Windows Defender Firewall API - GP       {0EFF663F-8B6E-4E6D-8182-087A8EAA29CB}
  // Windows Defender Firewall Driver         {D5E09122-D0B2-4235-ADC1-C89FAAAF1069}

  std::string providerName = "OpenTelemetry"; // supply unique instrumentation name here
  exporter::ETW::TracerProvider tp;

  // TODO: this code should fallback to MsgPack if TLD is not available
  auto tracer = tp.GetTracer(providerName, "TLD");

  // Span attributes
  Properties attribs =
  {
    {"attrib1", 1},
    {"attrib2", 2}
  };

  auto outerSpan = tracer->StartSpan("MySpanOuter", attribs);

  // Add first event
  std::string eventName1 = "MyEvent1";
  Properties event1 =
  {
    {"uint32Key", (uint32_t)1234},
    {"uint64Key", (uint64_t)1234567890},
    {"strKey", "someValue"}
  };
  EXPECT_NO_THROW(outerSpan->AddEvent(eventName1, event1));

  // Add second event
  std::string eventName2 = "MyEvent2";
  Properties event2 =
  {
    {"uint32Key", (uint32_t)9876},
    {"uint64Key", (uint64_t)987654321},
    {"strKey", "anotherValue"}
  };
  EXPECT_NO_THROW(outerSpan->AddEvent(eventName2, event2));


  // Create nested span. Note how we share the attributes here..
  // It is Okay to share or have your own attributes.
  auto innerSpan = tracer->StartSpan("MySpanInner", attribs);
  std::string eventName3= "MyEvent3";
    Properties event3 =
  {
    {"uint32Key", (uint32_t)9876},
    {"uint64Key", (uint64_t)987654321},
    // {"int32array", {{-1,0,1,2,3}} },
    {"tempString", getTemporaryValue() }
  };
  EXPECT_NO_THROW(innerSpan->AddEvent(eventName3, event3));

  EXPECT_NO_THROW(innerSpan->End());    // end innerSpan

  EXPECT_NO_THROW(outerSpan->End());    // end outerSpan

  EXPECT_NO_THROW(tracer->CloseWithMicroseconds(0));
}
/* clang-format on */

#endif
