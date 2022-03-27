namespace X
{

template<typename T, bool CheckExists = false>
void MoveLastInAndRemove(std::vector<T>& v, const T t)
{
    for (int i = 0; i < v.size(); ++i)
    {
        if (v[i] == t)
        {
            if constexpr (CheckExists)
                ALLOY_ASSERT(i != -1, "Archetype that doesn't exist was removed.");

            // If it's not the last one, move the last one in
            if (i != v.size() - 1) v[i] = v[v.size() - 1];
            v.pop_back();
            return;
        }
    }
}

} // namespace X
