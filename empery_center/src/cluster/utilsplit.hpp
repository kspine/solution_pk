#ifndef  UTIL_SPLIT_FILE
#define  UTIL_SPLIT_FILE

#include <string.h>
#include <vector>

namespace UTIL_SPLIT
{
	class UtilSplit
	{
	public:
		static std::vector<std::string> Split(std::string INstringSplit, std::string INSplitPattern)
		{
			std::string::size_type pos;
			std::vector<std::string> result;
			INstringSplit += INSplitPattern;
			std::uint32_t size = INstringSplit.size();
			for (std::uint32_t  index = 0; index < size; index++)
			{
				pos = INstringSplit.find(INSplitPattern, index);
				if (pos < size)
				{
					std::string str = INstringSplit.substr(index, pos - index);
					result.push_back(str);

					index = pos + INSplitPattern.size() - 1;
				}
			}
			return result;
		}

	private:
		UtilSplit();
		~UtilSplit();
	};
}
#endif//UTIL_SPLIT_FILE
