#include <boost/test/unit_test.hpp>
#include "in-memory-study.h"

namespace utf = boost::unit_test;
namespace tt = boost::test_tools;

struct StudyWithPenalty: public StudyWithBConLink
{
    using StudyWithBConLink::StudyWithBConLink;
};

BOOST_AUTO_TEST_SUITE(TESTS_BINDING_CONSTRAINTS_PENALTY)

BOOST_FIXTURE_TEST_CASE(hourly_BC_with_penalty_is_soft, StudyWithPenalty)
{
    setNumberMCyears(1);
    BC->setTimeGranularity(BindingConstraint::typeHourly);
    BC->operatorType(BindingConstraint::opEquality);
    BC->penalty(100.);
    TimeSeriesConfigurer(BC->RHSTimeSeries()).setDimensions(1).fillColumnWith(0, 0.);

    simulation->create();
    simulation->run();

    OutputRetriever output(simulation->rawSimu());
    BOOST_TEST(output.flow(link).hour(0) > 0.0);
}

BOOST_AUTO_TEST_SUITE_END()
