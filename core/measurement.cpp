#include "measurement.h"

OpMeasurement::OpMeasurement() {
	for (int i = 0; i < NR_OP_TYPE; ++i) {
		this->op_count_arr[i] = 0;
		this->rt_op_count_arr[i] = 0;
	}
	this->cur_progress = 0;
	this->finished = false;
}

void OpMeasurement::set_max_progress(long new_max_progress) {
	this->max_progress = new_max_progress;
}

void OpMeasurement::start_measure() {
	this->start_time = std::chrono::steady_clock::now();
	this->rt_time = std::chrono::steady_clock::now();
}

void OpMeasurement::finish_measure() {
	for (int i = 0; i < NR_OP_TYPE; ++i) {
		for (const auto& map_it : this->per_client_latency_list) {
			for (const auto& list_it : map_it.second[i]) {
				this->final_latency_tree[i].insert(list_it);
			}
		}
	}
	this->end_time = std::chrono::steady_clock::now();
	this->finished = true;
}

void OpMeasurement::record_op(OperationType type, double latency, int id) {
	++this->op_count_arr[type];
	++this->rt_op_count_arr[type];
	this->per_client_latency_list[id][type].push_back(latency);
}

void OpMeasurement::record_progress(long progress_delta) {
	this->cur_progress += progress_delta;
}

long OpMeasurement::get_op_count(OperationType type) {
	return this->op_count_arr[type];
}

double OpMeasurement::get_throughput(OperationType type) {
	long duration = std::chrono::duration_cast<std::chrono::microseconds>(
		this->end_time - this->start_time
	).count();
	return ((double) this->op_count_arr[type]) * 1000000 / duration;
}

void OpMeasurement::get_rt_throughput(double *throughput_arr) {
	std::chrono::steady_clock::time_point cur_time = std::chrono::steady_clock::now();
	long duration = std::chrono::duration_cast<std::chrono::microseconds>(
		cur_time - this->rt_time
	).count();
	for (int i = 0; i < NR_OP_TYPE; ++i) {
		throughput_arr[i] = ((double) this->rt_op_count_arr[i].exchange(0)) * 1000000 / duration;
	}
	this->rt_time = cur_time;
}

double OpMeasurement::get_progress_percent() {
	return ((double) this->cur_progress) / ((double) this->max_progress);
}

double OpMeasurement::get_latency_average(OperationType type) {
	double latency_sum = 0;
	avl_tree<double>::iterator iterator(this->final_latency_tree[type]);
	while (iterator) {
		latency_sum += *iterator;
		++iterator;
	}
	return latency_sum / (double) this->final_latency_tree[type].size();
}

double OpMeasurement::get_latency_percentile(OperationType type, float percentile) {
	return this->final_latency_tree[type].get_percentile(percentile);
}
