import express from "express";
import { mainPage } from "../client/page";
import axios from "axios";
import { tryGetAxios, tryPostAxios } from "@sergiyiva/retry-axios";

const baseApi = process.env.PICO_URL;
const apiKey = process.env.PICO_API_KEY;

const httpInstance = axios.create({
  baseURL: baseApi,
  headers: {
    "Authorization": apiKey
  }
});
const app = express();

const restAxiosArgs = [{}, {
  instance: httpInstance
}] as const;

app.use(express.json());

app.get("/", (_, res) => {
  res.status(200).send(mainPage);
});

app.get("/settings/config", async (_, res) => {
  const { data } = await tryGetAxios("/settings/config", ...restAxiosArgs);
  res.status(200).send(data);
});

app.post("/settings/:action", async (req, res) => {
  const body = req.body;
  const action = req.params.action;
  const { data } = await tryPostAxios(`/settings/${action}`, body, ...restAxiosArgs);
  res.status(200).send(data);
});

const port = process.env.PORT || 7777;
app.listen(port, () => {
  console.log(`Server started on port ${port}`);
});