#include "extra/SegmentTree.h" // Include the SegmentTree header
#include "data.h"              // Include the Data struct definition
#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include <numeric> // For std::accumulate (if not used directly in test)
#include <cmath>   // For std::sqrt

// Helper function to create sample Data objects (similar to avl.cpp test)
std::vector<Data> create_sample_data_for_testing_segment_tree() {
    std::vector<Data> samples;

    // Sample 1: Normal HTTP
    samples.emplace_back(
        1001, 1.5f, 10.0f, 1500.0f, 800.0f, 0.15f, 0.08f, 5.0f, 3.0f, 0.05f, 0.02f, 0.03f,
        15, 12, 7500, 4800, 64, 128, 0, 0, 65535, 100000, 200000, 32768, 500, 400, 1, 1200,
        5, 10, 3, 4, 8, 0, 1, 20, 6, false, false, false, Protocolo::TCP, State::FIN, Attack_cat::NORMAL, Servico::HTTP
    );

    // Sample 2: DoS UDP DNS (higher rate)
    samples.emplace_back(
        1002, 0.8f, 100.0f, 50000.0f, 100.0f, 0.01f, 0.5f, 20.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        500, 5, 250000, 250, 32, 0, 10, 0, 0, 0, 0, 0, 500, 50, 0, 0,
        200, 5, 150, 2, 10, 0, 0, 300, 10, false, true, true, Protocolo::UDP, State::INT, Attack_cat::DOS, Servico::DNS
    );

    // Sample 3: Reconnaissance ICMP Ping (lower rate, specific id to test interval)
    samples.emplace_back(
        1003, 12.5f, 0.5f, 60.0f, 60.0f, 2.0f, 2.0f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f,
        5, 5, 300, 300, 255, 255, 0, 0, 0, 0, 0, 0, 60, 60, 0, 0,
        1, 1, 1, 1, 1, 0, 0, 5, 1, false, false, true, Protocolo::ICMP, State::ECO, Attack_cat::RECONNAISSANCE, Servico::NOTHING
    );

    // Sample 4: FTP Backdoor (moderate rate)
    samples.emplace_back(
        1004, 60.2f, 2.0f, 200.0f, 150.0f, 0.8f, 1.2f, 10.0f, 15.0f, 0.2f, 0.1f, 0.1f,
        30, 25, 3000, 2000, 60, 60, 1, 1, 8192, 50000, 60000, 4096, 100, 80, 5, 50,
        2, 3, 1, 1, 2, 4, 0, 5, 2, true, true, true, Protocolo::TCP, State::CON, Attack_cat::BACKDOOR, Servico::FTP
    );

    // Sample 5: Another Normal HTTP (for interval testing)
    samples.emplace_back(
        1005, 2.0f, 15.0f, 2000.0f, 1000.0f, 0.10f, 0.07f, 6.0f, 4.0f, 0.06f, 0.03f, 0.04f,
        20, 18, 8000, 5000, 64, 128, 0, 0, 65535, 110000, 210000, 33000, 520, 410, 1, 1500,
        7, 12, 4, 5, 9, 0, 1, 22, 7, false, false, false, Protocolo::TCP, State::FIN, Attack_cat::NORMAL, Servico::HTTP
    );

    return samples;
}

void testSegmentTreeBasicOperations() {
    std::cout << "--- Test: SegmentTree Basic Operations (Insert, Find, Remove, TotalRate) ---\n";
    SegmentTree tree;
    std::vector<Data> test_data = create_sample_data_for_testing_segment_tree();

    // Insert all data
    for (const auto& data_item : test_data) {
        tree.insert(new Data(data_item)); // Pass a new Data object (or pointer from master_data_store in main)
    }
    std::cout << "Inserted " << test_data.size() << " elements.\n";

    // Test find
    const Data* found_data = tree.find(1002);
    assert(found_data != nullptr && found_data->id == 1002);
    std::cout << "Found data with ID 1002.\n";

    found_data = tree.find(9999); // Non-existent ID
    assert(found_data == nullptr);
    std::cout << "Confirmed ID 9999 not found.\n";

    // Test getTotalRate (sum of rates from all inserted data)
    // Manually calculate expected sumRate
    float expected_total_rate = 0.0f;
    for (const auto& data_item : test_data) {
        expected_total_rate += data_item.rate;
    }
    assert(std::abs(tree.getTotalRate() - expected_total_rate) < 0.001f);
    std::cout << "Total rate is correct: " << tree.getTotalRate() << "\n";

    // Test remove
    bool removed = tree.remove(1002);
    assert(removed);
    std::cout << "Removed data with ID 1002.\n";

    found_data = tree.find(1002);
    assert(found_data == nullptr);
    std::cout << "Confirmed ID 1002 no longer found after removal.\n";

    removed = tree.remove(9999); // Try removing non-existent
    assert(!removed);
    std::cout << "Confirmed non-existent ID 9999 not removed.\n";

    // Re-calculate total rate after removal
    expected_total_rate -= test_data[1].rate; // Remove rate of data with ID 1002
    assert(std::abs(tree.getTotalRate() - expected_total_rate) < 0.001f);
    std::cout << "Total rate is correct after removal: " << tree.getTotalRate() << "\n";

    std::cout << "--- Test: SegmentTree Basic Operations PASSED ---\n\n";
}

void testSegmentTreeStatisticalOperations() {
    std::cout << "--- Test: SegmentTree Statistical Operations (Interval-based) ---\n";
    SegmentTree tree;
    std::vector<Data> test_data = create_sample_data_for_testing_segment_tree();

    // Insert all data (ensure stable pointers for the tree - simulating main.cpp)
    std::vector<std::unique_ptr<Data>> owned_data;
    for (const auto& data_item : test_data) {
        owned_data.push_back(std::make_unique<Data>(data_item));
        tree.insert(owned_data.back().get());
    }
    std::cout << "Inserted " << owned_data.size() << " elements for statistical testing.\n";

    // Test with a small interval (e.g., last 3 items)
    int interval = 3;
    std::cout << "\nCalculating stats for last " << interval << " items:\n";

    // Manually get values for last 3 items for 'Rate'
    std::vector<float> last_3_rates;
    if (owned_data.size() >= 3) {
        last_3_rates.push_back(owned_data[owned_data.size() - 3]->rate);
        last_3_rates.push_back(owned_data[owned_data.size() - 2]->rate);
        last_3_rates.push_back(owned_data.back()->rate);
    } else {
        for(const auto& d_ptr : owned_data) {
            last_3_rates.push_back(d_ptr->rate);
        }
    }
    
    // Sort for median calculation
    std::vector<float> sorted_last_3_rates = last_3_rates;
    std::sort(sorted_last_3_rates.begin(), sorted_last_3_rates.end());

    // Calculate expected values
    float expected_avg_rate = std::accumulate(last_3_rates.begin(), last_3_rates.end(), 0.0f) / last_3_rates.size();
    float expected_min_rate = *std::min_element(last_3_rates.begin(), last_3_rates.end());
    float expected_max_rate = *std::max_element(last_3_rates.begin(), last_3_rates.end());
    float expected_median_rate = (last_3_rates.size() % 2 == 0) ? 
                                 (sorted_last_3_rates[sorted_last_3_rates.size()/2 - 1] + sorted_last_3_rates[sorted_last_3_rates.size()/2]) / 2.0f :
                                 sorted_last_3_rates[sorted_last_3_rates.size()/2];
    
    float sum_sq_diff_rate = 0.0f;
    for (float val : last_3_rates) {
        sum_sq_diff_rate += (val - expected_avg_rate) * (val - expected_avg_rate);
    }
    float expected_stddev_rate = (last_3_rates.size() > 0) ? std::sqrt(sum_sq_diff_rate / last_3_rates.size()) : 0.0f;


    // Get actual values from SegmentTree
    float actual_avg_rate = tree.getAverage(StatisticFeature::RATE, interval);
    float actual_stddev_rate = tree.getStdDev(StatisticFeature::RATE, interval);
    float actual_median_rate = tree.getMedian(StatisticFeature::RATE, interval);
    float actual_min_rate = tree.getMin(StatisticFeature::RATE, interval);
    float actual_max_rate = tree.getMax(StatisticFeature::RATE, interval);

    std::cout << "  Expected Avg Rate: " << expected_avg_rate << ", Actual Avg Rate: " << actual_avg_rate << "\n";
    std::cout << "  Expected StdDev Rate: " << expected_stddev_rate << ", Actual StdDev Rate: " << actual_stddev_rate << "\n";
    std::cout << "  Expected Median Rate: " << expected_median_rate << ", Actual Median Rate: " << actual_median_rate << "\n";
    std::cout << "  Expected Min Rate: " << expected_min_rate << ", Actual Min Rate: " << actual_min_rate << "\n";
    std::cout << "  Expected Max Rate: " << expected_max_rate << ", Actual Max Rate: " << actual_max_rate << "\n";


    // Assertions with a small tolerance for floating point comparisons
    assert(std::abs(actual_avg_rate - expected_avg_rate) < 0.001f);
    assert(std::abs(actual_stddev_rate - expected_stddev_rate) < 0.001f);
    assert(std::abs(actual_median_rate - expected_median_rate) < 0.001f);
    assert(std::abs(actual_min_rate - expected_min_rate) < 0.001f);
    assert(std::abs(actual_max_rate - expected_max_rate) < 0.001f);

    std::cout << "SegmentTree statistics for 'Rate' (last " << interval << " items) PASSED.\n";


    // Test with interval larger than data size (should use all data)
    interval = 100; // Larger than total items
    std::cout << "\nCalculating stats for last " << interval << " items (all data):\n";
    
    std::vector<float> all_rates;
    for(const auto& d_ptr : owned_data) {
        all_rates.push_back(d_ptr->rate);
    }

    std::vector<float> sorted_all_rates = all_rates;
    std::sort(sorted_all_rates.begin(), sorted_all_rates.end());

    expected_avg_rate = std::accumulate(all_rates.begin(), all_rates.end(), 0.0f) / all_rates.size();
    expected_min_rate = *std::min_element(all_rates.begin(), all_rates.end());
    expected_max_rate = *std::max_element(all_rates.begin(), all_rates.end());
    expected_median_rate = (all_rates.size() % 2 == 0) ? 
                                 (sorted_all_rates[sorted_all_rates.size()/2 - 1] + sorted_all_rates[sorted_all_rates.size()/2]) / 2.0f :
                                 sorted_all_rates[sorted_all_rates.size()/2];
    
    sum_sq_diff_rate = 0.0f;
    for (float val : all_rates) {
        sum_sq_diff_rate += (val - expected_avg_rate) * (val - expected_avg_rate);
    }
    expected_stddev_rate = (all_rates.size() > 0) ? std::sqrt(sum_sq_diff_rate / all_rates.size()) : 0.0f;

    actual_avg_rate = tree.getAverage(StatisticFeature::RATE, interval);
    actual_stddev_rate = tree.getStdDev(StatisticFeature::RATE, interval);
    actual_median_rate = tree.getMedian(StatisticFeature::RATE, interval);
    actual_min_rate = tree.getMin(StatisticFeature::RATE, interval);
    actual_max_rate = tree.getMax(StatisticFeature::RATE, interval);

    assert(std::abs(actual_avg_rate - expected_avg_rate) < 0.001f);
    assert(std::abs(actual_stddev_rate - expected_stddev_rate) < 0.001f);
    assert(std::abs(actual_median_rate - expected_median_rate) < 0.001f);
    assert(std::abs(actual_min_rate - expected_min_rate) < 0.001f);
    assert(std::abs(actual_max_rate - expected_max_rate) < 0.001f);
    
    std::cout << "SegmentTree statistics for 'Rate' (all items) PASSED.\n";


    std::cout << "--- Test: SegmentTree Statistical Operations PASSED ---\n\n";
}


int main() {
    std::cout << "Running SegmentTree tests...\n\n";
    testSegmentTreeBasicOperations();
    testSegmentTreeStatisticalOperations();
    std::cout << "All SegmentTree tests passed!\n";
    return 0;
}

