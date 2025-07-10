library(tidyverse)

data <- read_csv("innenhof_earthobservation.csv")

melted_data <- pivot_longer(
  data,
  cols = 2:5,
  names_to = "measurement",
  values_to = "value"
)

ggplot(melted_data, aes(x = timestamp, y = value, color = measurement)) +
  geom_line() +
  labs(
    title = "Innenhof Earth Observation"
  ) +
  theme_light()

ggsave("innenhof_earthobservation.pdf")
