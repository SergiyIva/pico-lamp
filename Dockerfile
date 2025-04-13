FROM oven/bun:1 AS base

FROM base AS deps

WORKDIR /app
COPY package.json bun.lock ./
RUN bun install --frozen-lockfile

FROM base AS runner

WORKDIR /app
COPY --from=deps /app/node_modules ./node_modules
COPY . .

ARG PORT
EXPOSE ${PORT}

CMD ["bun", "run", "start"]