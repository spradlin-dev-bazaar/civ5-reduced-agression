CREATE TABLE IF NOT EXISTS DllPlugins (File TEXT NOT NULL);
INSERT INTO DllPlugins (File) VALUES ('ReducedAggression.dll');

-- Percent of the AI's cheaper-unit advantage over Prince to cancel when it sizes up a human player (0 to 100; 0 turns it off).
INSERT INTO Defines (Name, Value) VALUES ('REDUCED_AGGRESSION_CANCEL_PERCENT', 100);
