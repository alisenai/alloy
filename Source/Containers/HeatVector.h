#pragma once

template<typename Type>
class HeatVector
{
public:
    HeatVector() : data()
    {}

    void EmplaceBack(const Type&& item)
    {
        data.emplace_back(item);
    }

    auto End() const
    {
        return data.end();
    }

    template<typename Predicate>
    auto Find(const Predicate&& predicate)
    {
        for (int i = 0; i < data.size(); ++i)
        {
            // If it's the item we want
            if (predicate(data[i]))
            {
                // Move item over by one
                if (i > 0)
                {
                    std::swap(data[i], data[i - 1]);
                    return data.begin() + i - 1;
                }
                return data.begin() + i;
            }
        }
        return data.end();
    }

    template<typename Predicate>
    void Each(const Predicate&& predicate) const
    {
        for (auto& item : data)
            predicate(item);
    }

private:
    std::vector<Type> data;
};
