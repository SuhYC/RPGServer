#pragma once

#include <random>

class ChanceEvaluator
{
public:
	// �̱��� ��ü ��ȯ
	static ChanceEvaluator& GetInstance() noexcept
	{
		// static��ü�� ����� �ѹ��� �ʱ�ȭ.
		static ChanceEvaluator instance;
		return instance;
	}

	// p = num_ / den_�� ��ǿ� ����
	// true : �߻�, false : �߻����� ����
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

		// [1,den_]�� �������� �յ��� �� ����
		std::uniform_int_distribution<> dis(1, den_);

		// 1~num_�� ���� ��ȯ : true, num_+1~den_�� ���� ��ȯ : false;
		return dis(gen) <= num_;
	}

	// baseValue_ * (1 - variationRange_) ~ baseValue_ * (1 + variationRange_) ������ ���� ����Ѵ�.
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
	// �ٸ� ��ġ���� �������� ���ϵ��� ����.
	ChanceEvaluator() : gen(rd())
	{

	}

	// ��������� �� ������Կ����� ����.
	ChanceEvaluator(const ChanceEvaluator&) = delete;
	ChanceEvaluator& operator=(const ChanceEvaluator&) = delete;

	std::random_device rd;
	std::mt19937 gen;
};