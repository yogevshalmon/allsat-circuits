#pragma once

/*
Simple input parser class, based on stackoverflow answer
*/
class InputParser
{
    public:
        InputParser (int &argc, char **argv){
            for (int i=1; i < argc; ++i)
                this->tokens.push_back(std::string(argv[i]));
        }

        void AppendParams(const std::vector<std::string>& params)
        {
            tokens.insert( tokens.end(), params.begin(), params.end() );
        }

        const std::string& getCmdOption(const std::string &option) const
        {
            std::vector<std::string>::const_iterator itr;
            itr =  std::find(this->tokens.begin(), this->tokens.end(), option);
            if (itr != this->tokens.end() && ++itr != this->tokens.end()){
                return *itr;
            }
            static const std::string empty_string("");
            return empty_string;
        }

        const std::string& getCmdOptionWDef(const std::string &option, const std::string &defVal) const
        {
            std::vector<std::string>::const_iterator itr;
            itr =  std::find(this->tokens.begin(), this->tokens.end(), option);
            if (itr != this->tokens.end() && ++itr != this->tokens.end()){
                return *itr;
            }

            // string no found return defVal
            return defVal;
        }

        const std::vector<std::pair<std::string, double>> getAllCmdOption(const std::string &option) const
        {
            std::vector<std::pair<std::string,double>> options = {};
            std::vector<std::string>::const_iterator itr = this->tokens.begin();
            while (itr != this->tokens.end())
            {   
                if (*itr == option)
                {
                    if (++itr != this->tokens.end())
                    {
                        if (++itr != this->tokens.end())
                        {
                            options.push_back({*(itr-1), std::stod(*itr)});
                        }
                    }
                }

                itr++;
            }
            return options;
        }

        bool cmdOptionExists(const std::string &option) const
        {
            return std::find(this->tokens.begin(), this->tokens.end(), option)
                   != this->tokens.end();
        }

        bool getBoolCmdOption(const std::string& option, bool defVal) const
        {
            bool retVal = defVal;
            const std::string& optionVal = getCmdOption(option);
            if (optionVal.empty())
            {
                return defVal;
            }

            if (optionVal == "true" || optionVal == "1")
                retVal = true;
            else if (optionVal == "false" || optionVal == "0")
                retVal = false;
            else
            {
                throw std::runtime_error("Cant parse value for option: " + option);
            }

            return retVal;
        }

        unsigned getUintCmdOption(const std::string& option, unsigned defVal) const
        {
            unsigned retVal = defVal;
            const std::string& optionVal = getCmdOption(option);
            if (optionVal.empty())
            {
                return defVal;
            }

            retVal = std::stoi(optionVal);

            if (retVal < 0)
            {
                throw std::runtime_error("Cant parse value for option: " + option);
            }

            return retVal;
        }

    private:
        std::vector <std::string> tokens;
};