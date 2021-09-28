#include "DydEvent.h"
#include "Tests.h"

#include <boost/filesystem.hpp>

testing::Environment* initXmlEnvironment();

testing::Environment* const env = initXmlEnvironment();

TEST(TestDydEvent, write) {
  using ContingencyElement = dfl::inputs::ContingencyElement;
  using ElementType = dfl::inputs::ContingencyElement::Type;

  std::string basename = "TestDydEvent";
  std::string dirname = "results";
  std::string filename = basename + ".dyd";

  boost::filesystem::path outputPath(dirname);
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directory(outputPath);
  }

  auto contingency = dfl::inputs::Contingency("TestContingency");
  // We need one element per case handled in DydEvent
  contingency.elements.emplace_back("TestBranch", ElementType::BRANCH);                       // buildBranchDisconnection (branch case)
  contingency.elements.emplace_back("TestGenerator", ElementType::GENERATOR);                 // signal: "generator_switchOffSignal2"
  contingency.elements.emplace_back("TestLoad", ElementType::LOAD);                           // signal: "switchOff2"
  contingency.elements.emplace_back("TestHvdcLine", ElementType::HVDC_LINE);                  // signal: "hvdc_switchOffSignal2"
  contingency.elements.emplace_back("TestShuntCompensator", ElementType::SHUNT_COMPENSATOR);  // buildNetworkStateDisconnection (general case)

  outputPath.append(filename);
  dfl::outputs::DydEvent dyd(dfl::outputs::DydEvent::DydEventDefinition(basename, outputPath.generic_string(), contingency));
  dyd.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}
