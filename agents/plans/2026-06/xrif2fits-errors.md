# Prompt
In utils/xrif2fits, we need to change the way error handling works.  Currently if it can't find a log or telem file with matching dates, it is a fatal error.  We instead want the fatal error to occur only if the top-level directory for that log/telem source doesn't exist.  E.g. if it is supposed to find `fwsci1` and there is no directory named `fwsci1`.  If the directory exists, but there are just no logs/telems in the date/time range, we want to notify the user but continue processing. The best solution would be to have the expected FITS header entries still be published, but with `NOT AVAILABLE` as the value.  

Review AGENTS.md then please analyze this problem, and then formulate a plan.  Update this document below with the plan, and do not make changes until I have approved it.  Do not modify this prompt above the "Plan" header below.  I have already created and switched to the feature branch for this.

# Plan
