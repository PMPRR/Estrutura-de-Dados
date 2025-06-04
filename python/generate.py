from pandas import Series
from sdv.single_table import CTGANSynthesizer

# Created with Gemini 2.5 pro
# Prompt foi perdido, mas era algo parecido com:
# """
#   Dado que eu possou um modelo de ctgan, crie um programa em python, usando a lib sdv
#   para carregar o modelo e gerar  dados sintéticos
#
# """

SAVED_MODEL_FILEPATH = "sdv_ctgan_unsw2_model.pkl"  # Path to your saved model
NUM_SYNTHETIC_SAMPLES_TO_GENERATE = (
    10000  # Number of synthetic samples you want to generate
)
OUTPUT_SYNTHETIC_DATA_FILEPATH = (
    "generated_synthetic_data.csv"  # Filepath to save the generated data
)

loaded_model = CTGANSynthesizer.load(filepath=SAVED_MODEL_FILEPATH)

synthetic_data = loaded_model.sample(num_rows=NUM_SYNTHETIC_SAMPLES_TO_GENERATE)

print(synthetic_data.tail())

id = range(NUM_SYNTHETIC_SAMPLES_TO_GENERATE)
synthetic_data["id"] = Series(id, index=synthetic_data.index)


cols = ["id"] + [col for col in synthetic_data.columns if col != "id"]
synthetic_data = synthetic_data[cols]

print(synthetic_data.info())

synthetic_data.to_csv("synthetic.csv", index=False)
