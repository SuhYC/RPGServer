#pragma once

#include <random>

class ChanceEvaluator
{
public:
	// 싱글톤 객체 반환
	static ChanceEvaluator& GetInstance() noexcept
	{
		// static객체를 사용해 한번만 초기화.
		static ChanceEvaluator instance;
		return instance;
	}

	// p = num_ / den_인 사건에 대해
	// true : 발생, false : 발생하지 않음
	// throws invalid_argument
	bool decide(const int num_, const int den_)
	{
		if (num_ > den_ ||
			num_ < 0 || den_ <= 0)
		{
			throw std::invalid_argument("");
		}

		if (num_ == 0)
		{
			return false;
		}

		if (num_ == den_)
		{
			return true;
		}

		// [1,den_]의 범위에서 균등한 수 생성
		std::uniform_int_distribution<> dis(1, den_);

		// 1~num_의 수가 반환 : true, num_+1~den_의 수가 반환 : false;
		return dis(gen) <= num_;
	}

	// baseValue_ * (1 - variationRange_) ~ baseValue_ * (1 + variationRange_) 범위의 수를 출력한다.
	long long CalDamage(const long long baseValue_, const float variationRange_)
	{
		long long st = static_cast<long long>(baseValue_ * (1.0f - variationRange_));
		long long end = static_cast<long long>(baseValue_ * (1.0f + variationRange_));

		if (st > end)
		{
			return 0;
		}

		std::uniform_int_distribution<> dis(st, end);

		return dis(gen);
	}

private:
	// 다른 위치에서 생성하지 못하도록 막음.
	ChanceEvaluator() : gen(rd())
	{

	}

	// 복사생성자 및 복사대입연산자 삭제.
	ChanceEvaluator(const ChanceEvaluator&) = delete;
	ChanceEvaluator& operator=(const ChanceEvaluator&) = delete;

	std::random_device rd;
	std::mt19937 gen;
};