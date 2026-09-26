#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>

//date and price struct for days worth of data in memory
struct PriceBar 
{
    std:: string date;
    double close;
};

//SIGNAL
enum class Signal
{
    HOLD,
    BUY,
    SELL
};

std:: vector<PriceBar> loadPriceData(std::string filename)
{
    std::vector<PriceBar> history;
    std::ifstream file(filename);

    //error if file not opened
    if (!file.is_open())
    {
        std::cout << "ERROR: File could not be opened: " << filename << std::endl;
        return history;
    }
    
    std:: string line;
    std::getline(file, line); //skip header

    //create PriceBars and add to history
    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string field;
        std::vector<std::string> fields;

        while (std::getline(ss, field, ','))
        {
            fields.push_back(field);
        }
       
        //index4 is the Close price values
        PriceBar bar;
        bar.date = fields[0];
        bar.close = std::stod(fields[4]);
        
        history.push_back(bar);
    }
    return history;
}

std::vector<double> findMovingAverage(std::vector<PriceBar> history, int window)
{
    std::vector<double> averages;
    int count = 0;
    while (count < history.size())
    {
        if (count < window-1)
        {
            //no valid average
            averages.push_back(-1.0);
        }
        else
        {
            //find average
            double total = 0.0;
            for (int i = 0; i < window; i++)
            {
                total = total + history[count-i].close;
            }
            averages.push_back(total/window);
        }
        count++;
    }
    return averages;
}

std::vector<Signal> generateSignals(std::vector<double> shortMAS, std::vector<double> longMAS)
{
    std::vector<Signal> signals;
    for (int i = 0; i < shortMAS.size(); i++)
    {
        //HOLD
        if (i == 0 || shortMAS[i-1] == -1.0 || longMAS[i-1] == -1.0)
        {
            signals.push_back(Signal::HOLD);
        }

        //BUYING
        else if (shortMAS[i-1] <= longMAS[i-1] && shortMAS[i] > longMAS[i])
        {
            signals.push_back(Signal::BUY);
        }

        //SELLING
        else if (shortMAS[i-1] >= longMAS[i-1] && shortMAS[i] < longMAS[i])
        {
            signals.push_back(Signal::SELL);
        }

        else
        {
            signals.push_back(Signal::HOLD);
        }
    }
    return signals;
}

std::string signalToString(Signal s)
{
    if (s == Signal::HOLD)
    {
        return "HOLD";
    }
    else if (s == Signal::BUY)
    {
        return "BUY";
    }
    else
    {
        return "SELL";
    }
}

//returns list of daily values of cash
std::vector<double> simulatePortfolio(std::vector<PriceBar> history, std::vector<Signal> signals, double initialCash, double feeRate, int& numTrades, double& winningTrades, int& completedTrades, std::vector<std::string>& tradesInfo)
{
    double sharesHeld = 0.0;
    double cash = initialCash;
    double buyPrice = 0.0;
    winningTrades = 0.0;
    completedTrades = 0;
    numTrades = 0;

    std::vector<double> dailyValues;

    for (int i = 0; i < history.size(); i++)
    {
        //ALL IN ALL OUT RULE
        double costMultiplier = 1 - feeRate;

        //buy
        if (sharesHeld == 0.0 && signals[i] == Signal::BUY)
        {
            sharesHeld = (cash * costMultiplier) / history[i].close;
            cash = 0.0;
            numTrades++;
            buyPrice = history[i].close;
            tradesInfo.push_back(history[i].date + " BUY @ " + std::to_string(history[i].close));
        }
        //sell
        else if (sharesHeld != 0.0 && signals[i] == Signal::SELL)
        {
            //if sell price >  buy price then WIN else LOSS
            std::string result = (history[i].close > buyPrice) ? "(WIN)" : "(LOSS)";

            if (history[i].close > buyPrice) 
            {
                winningTrades++;
            }
            completedTrades++;

            cash = (sharesHeld * history[i].close) * costMultiplier;
            sharesHeld = 0.0;
            numTrades++;

            tradesInfo.push_back(history[i].date + " SELL @ " + std::to_string(history[i].close) + " " + result);
        }

        //total for the day
        double total = cash + sharesHeld * history[i].close;
        dailyValues.push_back(total);
    }
    return dailyValues;
}

double calculateTotalReturn(std::vector<double> portfolioValues, double startingCash)
{
    double endCash = portfolioValues[portfolioValues.size()-1];
    double percentageChange = (endCash - startingCash) / startingCash;
    return (percentageChange * 100);
}

std::vector<double> calculateDailyReturns(std::vector<double> portfolioValues)
{
    std::vector<double> dailyReturns;

    for (int i = 1; i < portfolioValues.size(); i++)
    {
        double percentageChange = (portfolioValues[i] - portfolioValues[i-1]) / portfolioValues[i-1];
        dailyReturns.push_back(percentageChange);
    }
    return dailyReturns; //dailyReturns 1 element shorter than portfolioValues
}

double calculateVolatility(std::vector<double> dailyReturns)
{
    double dailyReturnTotal = 0.0;
    for (int i = 0; i < dailyReturns.size(); i++)
    {
        dailyReturnTotal = dailyReturnTotal + dailyReturns[i];
    }
    double mean = dailyReturnTotal/dailyReturns.size();

    double sumOfSquaresTotal = 0.0;
    for (int i = 0; i < dailyReturns.size(); i++)
    {
        double value = (dailyReturns[i] - mean) * (dailyReturns[i] - mean);
        sumOfSquaresTotal = sumOfSquaresTotal + value;
    }

    double variance = sumOfSquaresTotal / dailyReturns.size();
    double standardDeviation = std::sqrt(variance);
    return (standardDeviation * std::sqrt(252)); //annualised value
}

//assuming 0% risk-free rate
double calculateSharpeRatio(std::vector<double> dailyReturns)
{
    double meanDailyReturns = 0.0;
    for (int i = 0; i < dailyReturns.size(); i++)
    {
        meanDailyReturns = meanDailyReturns + dailyReturns[i];
    }

    double annualisedReturns = meanDailyReturns * 252; //annualised
    double volatility = calculateVolatility(dailyReturns);

    double sharpe = annualisedReturns / volatility;
    return sharpe;
}

double calculateMaxDrawdown(std::vector<double> portfolioValues)
{
    double peak = 0.0;
    double maxDrawDown = 0.0;

    for (int i = 0; i < portfolioValues.size(); i++)
    {
        //find running peak
        if (portfolioValues[i] > peak)
        {
            peak = portfolioValues[i];
        }

        //calculate drawdown
        double drawDown = (portfolioValues[i] - peak) / peak;
        if (drawDown < maxDrawDown)
        {
            maxDrawDown = drawDown;
        }
    }
    return (maxDrawDown * 100);
}

int main() 
{
    //max 2 decimal
    std::cout << std::fixed << std::setprecision(2);

    std::vector<PriceBar> history = loadPriceData("aapl_us_d.csv");
    std::vector<double> shortMA = findMovingAverage(history, 10); //10 days
    std::vector<double> longMA = findMovingAverage(history, 40); //40 days
    std::vector<Signal> signals = generateSignals(shortMA, longMA);
    std::vector<std::string> tradesInfo;

    double initialCash = 10000.0;
    int numTrades = 0;
    double feeRate = 0.001; //0.1% per trade
    double winningTrades = 0.0;
    int completedTrades = 0;

    std::vector<double> portfolioValues = simulatePortfolio(history, signals, initialCash, feeRate, numTrades, winningTrades, completedTrades, tradesInfo);

    double finalValue = portfolioValues[portfolioValues.size()-1];
    double totalReturn = calculateTotalReturn(portfolioValues, initialCash);
    std::vector<double> dailyReturns = calculateDailyReturns(portfolioValues);
    double volatility = calculateVolatility(dailyReturns);
    double sharpeRatio = calculateSharpeRatio(dailyReturns);
    double maxDrawDown = calculateMaxDrawdown(portfolioValues);
    
    double winRate = 0.0;
    if (completedTrades > 0) 
    {
     winRate = (winningTrades / completedTrades) * 100;
    }

    //INTRODUCTION
    std::cout << "\n==================================================\nC++ Trading Backtester v1.0 by TIANA CHU\n==================================================\n\nI have created this tool to simulate a moving-average crossover strategy on historical price data (AAPL). \n\n" << std::endl;

    //MAIN REPORT
    std::cout << "BACKTEST REPORT: \n\nAsset: AAPL\nPeriod: " << history[0].date << " to " << history[history.size()-1].date << "\nStarting Capital: $" << initialCash << "\nEnding Value: $" << finalValue << "\nTotal Return: " << totalReturn << "%\nFee Rate: " << feeRate * 100 << "%\nAnnual Volatility: " << volatility * 100 << "%\nSharpe Ratio: " << sharpeRatio << "\nMaximum Drawdown: " << maxDrawDown << "%\nNumber of Trades: " << numTrades << "\nWin Rate: " << winRate << "%\n\n==================================================" << std::endl;

    //TRADE LOG
    std::cout << "\nTRADE LOG:" << std::endl;

    for (int i = 0; i < tradesInfo.size(); i++)
    {
        std::cout << tradesInfo[i] << "\n" << std::endl;
    }
    return 0;
}
