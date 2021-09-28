#include "Contingencies.h"
#include "ParEvent.h"
#include "Tests.h"

#include <boost/filesystem.hpp>

testing::Environment* initXmlEnvironment();

testing::Environment* const env = initXmlEnvironment();

TEST(TestParEvent, write) {
  using ContingencyElement = dfl::inputs::ContingencyElement;
  using ElementType = dfl::inputs::ContingencyElement::Type;

  std::string basename = "TestParEvent";
  std::string dirname = "results";
  std::string filename = basename + ".par";

  boost::filesystem::path outputPath(dirname);
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directory(outputPath);
  }

  auto contingency = dfl::inputs::Contingency("TestContingency");
  // We need the three of them to check the three cases that can be generated
  contingency.elements.emplace_back("TestBranch", ElementType::BRANCH);                       // buildBranchDisconnection (branch case)
  contingency.elements.emplace_back("TestGenerator", ElementType::GENERATOR);                 // buildEventSetPointBooleanDisconnection
  contingency.elements.emplace_back("TestShuntCompensator", ElementType::SHUNT_COMPENSATOR);  // buildEventSetPointRealDisconnection (general case)

  outputPath.append(filename);
  dfl::outputs::ParEvent par(dfl::outputs::ParEvent::ParEventDefinition(basename, outputPath.generic_string(), contingency, std::chrono::seconds(80)));
  par.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}
