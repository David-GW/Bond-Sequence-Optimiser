# Bond Sequence Optimiser

## Program Overview

Given holding period returns (HPR, the realised percentage return from buying now and selling at maturity, **not** annualised yields such as APR/APY) for bonds of various tenors, this program computes the optimal buying strategies to maximise the HPR over the whole period.

The program takes a CSV of the form:

```
Tenor,0,1,2,3,4,5,6,7,8,9,10,11
3,0.0102,0.0099,0.0105,0.0096,0.0101,0.0108,0.0094,0.0098,0.0100,0.0106,0.0097,0.0103
6,0.0205,0.0198,0.0210,0.0192,0.0202,0.0218,0.0188,0.0195,0.0200,0.0212,0.0193,0.0206
12,0.0401,0.0410,0.0430,0.0395,0.0415,0.0440,0.0390,0.0405,0.0420,0.0435,0.0400,0.0428
```

and computes the optimal buying strategies to maximise returns.

Here we have provided 12 months of data for bonds with tenors of 3, 6, and 12 months, but the horizon length and number of tenors are limited only by time and memory.

If we request the top 10 results, and print to the terminal, the program returns:

```
1. 4.10%: b6,b3,b3
2. 4.05%: b3,b6,b3
3. 4.04%: b3,b3,b3,b3
4. 4.01%: b12
5. 3.97%: b6,b6
6. 3.91%: b3,b3,b6
7. 3.25%: w2,b3,b6,w1
8. 3.22%: w2,b3,b3,w1,b3
9. 3.22%: b3,w2,b6,w1
10. 3.19%: b3,w2,b3,w1,b3
```

where b denotes "buy" and w denotes "wait". It can also export this data to a CSV.

This tells us that the optimal strategy over 12 months would be to buy a 6-month bond, a 3-month bond, then a final 3-month bond, which would yield an HPR of 4.10%.

There are also strategies where waiting is beneficial, for example the 7th most optimal strategy requires waiting 2 months before making purchases.

Finally, the program can compute the total number of possible strategies for those curious; in this toy example there are only 92, but longer horizons and/or adding shorter tenors can easily yield billions of possibilities.

## CSV Requirements

- The extension must be either `.csv` or `.txt`.
- The first cell of the CSV must be "Tenor" (case-insensitive).
- Months must be contiguous from 0.
- Tenors must be positive integers.
- There can be no missing HPRs.
- Blank lines are ignored.

## Build/Run
1. `cmake -S . -B build -G Ninja`
2. `cmake --build build`
3. `./build/Bond_Sequence_Optimiser`

---

## Implementation Details

The program works on two principles: dynamic programming and *k*-way merging.

### Dynamic Programming

As noted above, non-trivial data can easily yield billions of possibilities, far too many to recursively enumerate and sort naïvely.

Instead, if we are asked for the top *k* results, we only store the top *k* cumulative return factors (CRFs) for each month (this is effectively the "return so far", calculated by mulitplying (1+HPR) at each purchase), as well as the buying strategies to arrive at them. To find the current month's results, we look backwards at the possibilities from previous months (with an initial value seeded at a CRF of 1).

For example, say that we were at month 10. With our example data set, we would look back at the top results from month 7 and see what buying a 3-month bond then would yield, and the same for month 4 with a 6-month bond. This is the dynamic programming component.

### *k*-way Merging

There is however a second consideration: say that we had a non-trivial data set, and wanted the top million results (we may want to analyse the distribution of optimal results for example). Furthermore, we could be 50 months out, with data from 1, 9 and 24 month tenors to consider as well.

This is where we apply *k*-way merging: rather than working out the 6 million CRFs (calculating the CRF from buying a 1-month bond from the top million results at month 49, a 3-month bond from the top million results at month 47, ...) and sorting them, we opt for a more efficient strategy.

We take the top result from each of the 6 possible months (49, 47, 44, 41, 38, and 26), and work out the CRF obtained from buying a bond of the appropriate tenor at the end of that buying strategy. This leaves us with just 6 values. We take the top value from that list and add it to our list of top results for month 50.

Say, for example, that this top result arose from buying a 3-month bond at month 47. We then go to the second best result on that list, work out the CRF after buying a 3-month bond from there, and add this to the list of the 5 remaining values from before. We then select the new largest, and repeat this process until we have our top million results.

### Complexity

The naïve approach would be to work out all possible paths, sort them, and choose the top *k*. For *n* tenors and *m* months of data, this will be *O*(*m*·(*n*+1)^*m*). By contrast, our approach reduces this to *O*(*m*·(*n*+*k*)·log(*n*+1)).

Assuming that the number of tenors (*n*) and results requested (*k*) is fixed, this means that our runtime grows linearly (*O*(*m*)) rather than exponentially as the horizon expands.