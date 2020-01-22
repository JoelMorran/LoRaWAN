WITH cte
     AS (SELECT *,
                row_number() OVER (ORDER BY DeviceID) AS rn
         FROM   TeamF)
DELETE FROM cte
WHERE  rn > 389