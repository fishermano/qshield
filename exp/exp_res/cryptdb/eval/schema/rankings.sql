use benchmark;

grant select, insert, update, delete on benchmark.* to 'root'@'letmein' identified by 'root';

SET FOREIGN_KEY_CHECKS=0;

-- -------------------------
-- Table struct for rankings
-- -------------------------
DROP TABLE IF EXISTS `rankings`;
CREATE TABLE `rankings` (
  `page_url` varchar(255) DEFAULT NULL,
  `page_rank` int DEFAULT NULL,
  `avg_duration` int DEFAULT NULL
) ENGINE=InnoDB default CHARSET=utf8 ROW_FORMAT=COMPACT;
